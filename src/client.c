#include <lua5.4/lauxlib.h>
#include <lua5.4/lualib.h>
#include <pthread.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "client.h"

struct termios term;
fd_set fds;

// Write a u32 representing the size of the message followed by the message
ssize_t write_message(int sock, char *buf, size_t len) {
  uint32_t size = len;
  if (write(sock, &size, sizeof(uint32_t)) != sizeof(uint32_t)) {
    return -1;
  }
  return write(sock, buf, len);
}

ssize_t write_string(int sock, char *buf) {
  return write_message(sock, buf, strlen(buf));
}

void enable_mouse_tracking() {
  write(STDOUT_FILENO, "\033[?1003h", 8); // Mouse tracking
  // write(STDOUT_FILENO, "\033[?1006h", 8); // Extended mouse tracking
}

void disable_mouse_tracking() {
  write(STDOUT_FILENO, "\033[?1003l", 8); // Mouse tracking
  // write(STDOUT_FILENO, "\033[?1006l", 8); // Extended mouse tracking
}

// TODO: This is a hack
bool rawMode = false;
void enter_raw_mode() {
  printf("\033[?1049h"); // Alternate screen
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
  enable_mouse_tracking();
  rawMode = true;
}

void reset_mode() {
  disable_mouse_tracking();
  term.c_lflag |= ICANON | ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
  rawMode = false;
  printf("\033[?1049l"); // Normal screen
  printf("\033[2J");     // Clear screen
}

bool receive_message(int sock) {
  // Get size of message
  char buf[sizeof(uint64_t)];
  int read_size = recv(sock, buf, sizeof(uint64_t), 0);
  if (read_size == 0) {
    puts("Server disconnected");
    return false;
  } else if (read_size == -1) {
    perror("recv failed");
    return false;
  }

  uint64_t n = 0;
  memcpy(&n, buf, sizeof(uint64_t));
  n -= sizeof(uint64_t);

  // Get message
  char server_reply[n];
  while (n > 0) {
    read_size = recv(sock, server_reply, n, 0);
    if (read_size == 0 && n > 0) {
      puts("Server disconnected");
      return false;
    } else if (read_size == -1) {
      perror("recv failed");
      return false;
    }
    if (rawMode) {
      write(STDOUT_FILENO, server_reply, read_size);
    }
    n -= read_size;
  }

  return true;
}

void send_size(int sock) {
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  uint32_t width = ws.ws_col;
  uint32_t height = ws.ws_row;
  height--; // Make room for status bar

  char buf[9];
  bzero(buf, 9);
  buf[0] = 's';
  memcpy(buf + 1, &width, sizeof(uint32_t));
  memcpy(buf + 5, &height, sizeof(uint32_t));
  write_message(sock, buf, 9);
}

void handle_binding(size_t numRead, char *buf, int sock, char *command,
                    char *binding) {
  if (numRead == strlen(binding) &&
      strncmp(buf + 1, binding, strlen(binding)) == 0) {
    write_string(sock, command);
  }
}

char *readline_text;
int add_readline_history() {
  rl_insert_text(readline_text);
  return 0;
}

void handle_rename(int sock, char *prompt, char *default_name, char *command) {
  reset_mode();
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  uint32_t height = ws.ws_row;

  printf("\033[0m");            // Reset colors
  printf("\033[%d;1H", height); // Move to bottom of screen
  printf("\033[K");             // Clear line

  readline_text = default_name;
  char *input = readline(prompt);
  char buf[32];

  if (strlen(input) > 0) {
    sprintf(buf, "%s %s", command, input);
    write_string(sock, buf);
  }
  enter_raw_mode();
  write_string(sock, "cReRender");
  write_string(sock, "cRenderBar");
}

int lua_write_string(lua_State *L) {
  int sock = lua_tointeger(L, 1);
  const char *str = lua_tostring(L, 2);
  write_string(sock, (char *)str);
  return 0;
}

void register_functions(lua_State *L) {
  lua_register(L, "write_string", lua_write_string);
}

lua_State *L = NULL;
bool handle_lua_binding(int numRead, char *buf, int sock, Mode mode) {
  if (!L) {
    char *filename = "bindings.lua";
    struct stat buffer;
    if (stat(filename, &buffer) == 0) {
      L = luaL_newstate();
      luaL_openlibs(L);
      register_functions(L);
      luaL_dofile(L, filename);
    }
  }

  if (!L) {
    return false;
  }

  if (mode == MODE_CONTROL) {
    lua_getglobal(L, "handle_binding_control");
    if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1);
      return false;
    }

    lua_pushinteger(L, sock);

    char cmd[numRead + 1];
    memcpy(cmd, buf + 1, numRead);
    cmd[numRead] = '\0';
    lua_pushstring(L, cmd);
    lua_call(L, 2, 1);
    if (lua_isboolean(L, -1)) {
      bool ret = lua_toboolean(L, -1);
      lua_pop(L, 1);
      if (ret) {
        return true;
      }
    }
  } else if (mode == MODE_NORMAL) {
    lua_getglobal(L, "handle_binding_normal");
    if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1);
      return false;
    }

    lua_pushinteger(L, sock);

    char cmd[numRead + 1];
    memcpy(cmd, buf + 1, numRead);
    cmd[numRead] = '\0';
    lua_pushstring(L, cmd);
    lua_call(L, 2, 1);
    if (lua_isboolean(L, -1)) {
      bool ret = lua_toboolean(L, -1);
      lua_pop(L, 1);
      if (ret) {
        return true;
      }
    }
  }

  return false;
}

char *execute_command(char *cmd) {
  FILE *fp;
  char path[1035];

  fp = popen(cmd, "r");
  if (fp == NULL) {
    printf("Failed to run command\n");
    exit(EXIT_FAILURE);
  }

  char *ret = NULL;
  while (fgets(path, sizeof(path) - 1, fp) != NULL) {
    ret = strdup(path);
  }

  pclose(fp);
  return ret;
}

void *handle_events(void *socket_desc) {
  int sock = *(int *)socket_desc;
  rl_startup_hook = (rl_hook_func_t *)add_readline_history;
  enter_raw_mode();

  Mode mode = MODE_NORMAL;
  write_string(sock, "cModeNormal");

  char buf[32];
  buf[0] = 'e';
  while (1) {
    ssize_t n = read(STDIN_FILENO, buf + 1, 6);
    if (n <= 0) {
    }

    if (mode == MODE_NORMAL) {
      if (handle_lua_binding(n, buf, sock, MODE_NORMAL)) {
      } else if (n == 1 && buf[1] == 1) { // Ctrl-A
        mode = MODE_CONTROL;
        write_string(sock, "cModeControl");
      } else if (n == 1 && buf[1] == 10) { // Enter
        buf[1] = 13;                       // Change newline to carriage return
        write_message(sock, buf, n + 1);
      } else {
        write_message(sock, buf, n + 1);
      }
    } else if (mode == MODE_CONTROL || mode == MODE_CONTROL_STICKY) {
      // TODO: Detect and ignore mouse events in control mode
      handle_binding(n, buf, sock, "cScrollUp", "\033[5~");   // Page up
      handle_binding(n, buf, sock, "cScrollDown", "\033[6~"); // Page down

      handle_lua_binding(n, buf, sock, MODE_CONTROL);

      if (n == 1 && buf[1] == 'o') {
        reset_mode();
        char *ret = execute_command("./scripts/list_commands.sh");
        write_string(sock, ret);
        free(ret);
        enter_raw_mode();
      }

      if (n == 1 && buf[1] == 't') {
        reset_mode();
        // char *ret = execute_command("./scripts/change_statusbar.sh");
        // write_string(sock, ret);
        // free(ret);
        write_string(sock, "cSelectStatus 0");
        enter_raw_mode();
      }

      if (n == 1 && buf[1] == 'c') {
        handle_rename(sock, "Name of New Window: ", "", "cCreateNamed");
      }

      if (n == 1 && buf[1] == ',') {
        handle_rename(sock, "Rename Window: ", "", "cRenameWindow");
      }

      if (n == 1 && buf[1] == '$') {
        handle_rename(sock, "Rename Session: ", "", "cRenameSession");
      }

      if (n == 1 && buf[1] == 'g') {
        reset_mode();
        system("./scripts/show_log.sh");
        enter_raw_mode();
        write_string(sock, "cLog");
      }

      // TODO: Move to Lua
      if (n == 1 && buf[1] == 'n') {
        send_size(sock);
      }

      // TODO: Move to Lua
      if (n == 1 && buf[1] == 'p') {
        send_size(sock);
      }

      if (n == 1 && buf[1] == 's') {
        mode = MODE_CONTROL_STICKY;
        write_string(sock, "cModeControlSticky");
      }

      if (mode == MODE_CONTROL_STICKY) {
        if (n == 1 && buf[1] == 1) { // Ctrl-A
          mode = MODE_NORMAL;
          write_string(sock, "cModeNormal");
        }
      }

      if (mode == MODE_CONTROL) {
        mode = MODE_NORMAL;
        write_string(sock, "cModeNormal");
      }

      write_string(sock, "cReRender");
      write_string(sock, "cRenderBar");
    }
  }

  reset_mode();
  return 0;
}

int resize_socket;
void handle_resize() { send_size(resize_socket); }

void *receive_messages(void *socket_desc) {
  int sock = *(int *)socket_desc;

  resize_socket = sock;
  signal(SIGWINCH, handle_resize);

  send_size(sock);

  while (1) {
    if (!receive_message(sock)) {
      break;
    }
  }
  close(sock);

  return 0;
}

void handle_sigint(int sig) {
  write_message(resize_socket, "e\x03", 2);
  signal(sig, handle_sigint);
}

int start_client(int sock) {
  signal(SIGINT, handle_sigint);

  write(STDOUT_FILENO, "\033[?1049h", 8); // Alternate screen

  pthread_t thread;
  pthread_create(&thread, NULL, handle_events, (void *)&sock);

  receive_messages((void *)&sock);

  pthread_cancel(thread);
  pthread_join(thread, NULL);

  write(STDOUT_FILENO, "\033[?1049l", 8); // Normal screen
  write(STDOUT_FILENO, "\033[?25h", 6);   // Make cursor visible

  close(sock);

  return EXIT_SUCCESS;
}
