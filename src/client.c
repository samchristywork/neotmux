#include <arpa/inet.h>
#include <pthread.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>

struct termios term;
enum Mode { MODE_NORMAL, MODE_CONTROL, MODE_CONTROL_STICKY };
fd_set fds;

// Write a u32 representing the size of the message followed by the message
ssize_t write_message(int sock, char *buf, size_t len) {
  uint32_t size = len;
  if (write(sock, &size, sizeof(uint32_t)) != sizeof(uint32_t)) {
    return -1;
  }
  return write(sock, buf, len);
}

int ctrl_c_socket;
void handle_sigint(int sig) {
  char buf[2];
  buf[0] = 'e';
  buf[1] = 3;
  write_message(ctrl_c_socket, buf, 2);

  signal(sig, handle_sigint);
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
}

bool receive_message(int sock) {
  FD_ZERO(&fds);
  FD_SET(sock, &fds);

  char server_reply[2000];
  int retval = select(sock + 1, &fds, NULL, NULL, NULL);
  if (retval == -1) {
    perror("select()");
    return false;
  } else if (retval) {
    int read_size = recv(sock, server_reply, 2000, 0);
    if (read_size == 0) {
      puts("Server disconnected");
      return false;
    } else if (read_size == -1) {
      perror("recv failed");
      return false;
    } else if (read_size == 1 && strncmp(server_reply, "e", 1) == 0) {
      return false;
    }
    if (rawMode) {
      write(STDOUT_FILENO, server_reply, read_size);
      fflush(stdout);
    }
  } else {
    puts("Timeout (This should not happen)");
    return false;
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

void handle_binding(int numRead, char *buf, int sock, char *command,
                    char *binding) {
  if (numRead == strlen(binding) &&
      strncmp(buf + 1, binding, strlen(binding)) == 0) {
    write_message(sock, command, strlen(command));
  }
}

char *readline_text;
void add_readline_history() { rl_insert_text(readline_text); }

// TODO: Deduplicate this
void handle_create_window(int sock) {
  reset_mode();
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  uint32_t height = ws.ws_row;

  printf("\033[0m");            // Reset colors
  printf("\033[%d;1H", height); // Move to bottom of screen
  printf("\033[K");             // Clear line

  readline_text = "Window";
  char *input = readline("Name of New Window: ");
  char buf[32];
  sprintf(buf, "cRenameWindow %s", input);
  write_message(sock, "cCreate", 7);
  write_message(sock, buf, strlen(buf));
  enter_raw_mode();
}

void handle_window_rename(int sock) {
  reset_mode();
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  uint32_t height = ws.ws_row;

  printf("\033[0m");            // Reset colors
  printf("\033[%d;1H", height); // Move to bottom of screen
  printf("\033[K");             // Clear line

  readline_text = "Window"; // TODO: Get this from server
  char *input = readline("Rename Window: ");
  char buf[32];
  sprintf(buf, "cRenameWindow %s", input);
  write_message(sock, buf, strlen(buf));
  enter_raw_mode();
}

void handle_session_rename(int sock) {
  reset_mode();
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  uint32_t height = ws.ws_row;

  printf("\033[0m");            // Reset colors
  printf("\033[%d;1H", height); // Move to bottom of screen
  printf("\033[K");             // Clear line

  readline_text = "Session"; // TODO: Get this from server
  char *input = readline("Rename Session: ");
  char buf[32];
  sprintf(buf, "cRenameSession %s", input);
  write_message(sock, buf, strlen(buf));
  enter_raw_mode();
}

void handle_events(int sock) {
  rl_startup_hook = (rl_hook_func_t *)add_readline_history;
  enter_raw_mode();

  int mode = MODE_NORMAL;
  char buf[32];
  buf[0] = 'e';
  while (1) {
    ssize_t n = read(STDIN_FILENO, buf + 1, 6);
    if (n <= 0) {
      exit(EXIT_SUCCESS);
    }

    if (mode == MODE_NORMAL) {
      // Alt+hjkl
      if (n == 2 && buf[1] == 27 && buf[2] == 'h') {
        write_message(sock, "cLeft", 5);
      } else if (n == 2 && buf[1] == 27 && buf[2] == 'l') {
        write_message(sock, "cRight", 6);
      } else if (n == 2 && buf[1] == 27 && buf[2] == 'k') {
        write_message(sock, "cUp", 3);
      } else if (n == 2 && buf[1] == 27 && buf[2] == 'j') {
        write_message(sock, "cDown", 5);
      } else if (n == 1 && buf[1] == 1) { // Ctrl-A
        mode = MODE_CONTROL;
      } else if (n == 1 && buf[1] == 10) { // Enter
        buf[1] = 13;                       // Change newline to carriage return
        write_message(sock, buf, n + 1);
      } else {
        write_message(sock, buf, n + 1);
      }
    } else if (mode == MODE_CONTROL || mode == MODE_CONTROL_STICKY) {
      // TODO: Change to lua
      handle_binding(n, buf, sock, "cSplit", "|");
      handle_binding(n, buf, sock, "cVSplit", "_");
      handle_binding(n, buf, sock, "cSplit", "\"");
      handle_binding(n, buf, sock, "cVSplit", "%");
      handle_binding(n, buf, sock, "cList", "i");
      handle_binding(n, buf, sock, "cCycleStatus", "y");
      handle_binding(n, buf, sock, "cCreate", "e");
      handle_binding(n, buf, sock, "cReloadLua", "r");
      handle_binding(n, buf, sock, "cLeft", "h");
      handle_binding(n, buf, sock, "cDown", "j");
      handle_binding(n, buf, sock, "cUp", "k");
      handle_binding(n, buf, sock, "cRight", "l");
      handle_binding(n, buf, sock, "cZoom", "z");
      handle_binding(n, buf, sock, "cEven_Horizontal", "1");
      handle_binding(n, buf, sock, "cEven_Vertical", "2");
      handle_binding(n, buf, sock, "cMain_Horizontal", "3");
      handle_binding(n, buf, sock, "cMain_Vertical", "4");
      handle_binding(n, buf, sock, "cTiled", "5");
      handle_binding(n, buf, sock, "cCustom", "6");
      handle_binding(n, buf, sock, "cNext", "n");
      handle_binding(n, buf, sock, "cPrev", "p");
      handle_binding(n, buf, sock, "cScrollUp", "\033[5~");   // Page up
      handle_binding(n, buf, sock, "cScrollDown", "\033[6~"); // Page down
      handle_binding(n, buf, sock, "cLeft", "\033[D");        // Left
      handle_binding(n, buf, sock, "cRight", "\033[C");       // Right
      handle_binding(n, buf, sock, "cUp", "\033[A");          // Up
      handle_binding(n, buf, sock, "cDown", "\033[B");        // Down

      if (n == 1 && buf[1] == 'c') {
        handle_create_window(sock);
      } else if (n == 1 && buf[1] == ',') {
        handle_window_rename(sock);
      } else if (n == 1 && buf[1] == '$') {
        handle_session_rename(sock);
      }

      if (n == 1 && buf[1] == 'n') {
        send_size(sock);
      }

      if (n == 1 && buf[1] == 'p') {
        send_size(sock);
      }

      if (n == 1 && buf[1] == 's') {
        mode = MODE_CONTROL_STICKY;
      }

      if (n == 1 && buf[1] == 'q') {
        break;
      }

      if (mode == MODE_CONTROL_STICKY) {
        if (n == 1 && buf[1] == 1) { // Ctrl-A
          mode = MODE_NORMAL;
        }
      }

      if (mode == MODE_CONTROL) {
        mode = MODE_NORMAL;
      }
    }
  }

  reset_mode();
}

int resize_socket;
void handle_resize(int sig) { send_size(resize_socket); }

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
  exit(EXIT_SUCCESS);
}

int start_client(int port, char *name) {
  signal(SIGINT, handle_sigint);
  int sock;

  // struct sockaddr_in server;
  // server.sin_addr.s_addr = inet_addr("127.0.0.1");
  // server.sin_family = AF_INET;
  // server.sin_port = htons(port);
  // sock = socket(AF_INET, SOCK_STREAM, 0);

  mkdir("/tmp/ntmux-1000", 0777);
  struct sockaddr_un server;
  server.sun_family = AF_UNIX;
  snprintf(server.sun_path, sizeof(server.sun_path), "/tmp/ntmux-1000/%s.sock",
           name);
  sock = socket(AF_UNIX, SOCK_STREAM, 0);

  ctrl_c_socket = sock;
  if (sock == -1) {
    perror("Could not create socket");
    close(sock);
    return EXIT_FAILURE;
  }

  if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("connect failed. Error");
    close(sock);
    return EXIT_FAILURE;
  }

  write(STDOUT_FILENO, "\033[?1049h", 8); // Alternate screen

  pthread_t thread;
  pthread_create(&thread, NULL, receive_messages, (void *)&sock);

  handle_events(sock);

  pthread_cancel(thread);
  pthread_join(thread, NULL);
  close(sock);

  write(STDOUT_FILENO, "\033[?1049l", 8); // Normal screen

  return EXIT_SUCCESS;
}
