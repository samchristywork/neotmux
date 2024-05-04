#include <arpa/inet.h>
#include <ctype.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "command.h"
#include "layout.h"
#include "mouse.h"
#include "plugin.h"
#include "render.h"

bool dirty = true; // TODO: Dirty should be on a per-pane basis
Neotmux *neotmux;

// Read a u32 representing the size of the message followed by the message
ssize_t read_message(int sock, char *buf, size_t len) {
  // call_function(neotmux->lua, "helloWorld");
  uint32_t size;
  if (read(sock, &size, sizeof(uint32_t)) != sizeof(uint32_t)) {
    return -1;
  }
  return read(sock, buf, size);
}

int die_socket_desc = -1;
void die(char *msg) {
  if (msg) {
    printf("%s\n", msg);
  }
  close(die_socket_desc);
  printf("Exiting...\n");
  exit(EXIT_SUCCESS);
}

void handle_ctrl_c(int sig) {
  signal(SIGINT, handle_ctrl_c);
  die("Ctrl-C");
}

struct sockaddr_in setup_server(int socket_desc, int port) {
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
    puts("bind failed");
    exit(EXIT_FAILURE);
  }

  return server;
}

void wait_for_connections(int socket_desc) {
  listen(socket_desc, 3);
  puts("Waiting for incoming connections...");
}

int accept_connection(int socket_desc, struct sockaddr_in client) {
  int c = sizeof(struct sockaddr_in);
  int socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
  if (socket < 0) {
    perror("accept failed");
    exit(EXIT_FAILURE);
  }
  return socket;
}

void reorder_windows() {
  for (int i = 0; i < neotmux->session_count; i++) {
    Session *session = &neotmux->sessions[i];
    for (int j = 0; j < session->window_count; j++) {
      Window *w = &session->windows[j];
      if (w->pane_count == 0) {
        for (int k = j; k < session->window_count - 1; k++) {
          session->windows[k] = session->windows[k + 1];
        }
        session->window_count--;
        j--;
        if (session->current_window >= session->window_count) {
          session->current_window = session->window_count - 1;
        }
      }
    }
  }
}

void delete_window(Window *w) {
  w->pane_count = 0;
  w->current_pane = -1;

  reorder_windows();
}

void reorder_panes(Window *w) {
  printf("Reordering panes\n");
  for (int i = 0; i < w->pane_count; i++) {
    Pane *p = &w->panes[i];
    if (p->process.closed) {
      for (int j = i; j < w->pane_count - 1; j++) {
        w->panes[j] = w->panes[j + 1];
      }
      w->pane_count--;
      i--;
    }
  }
  if (w->current_pane >= w->pane_count) {
    w->current_pane = w->pane_count - 1;
  }

  if (w->zoom >= w->pane_count) {
    w->zoom = -1;
  }

  if (w->pane_count == 0) {
    delete_window(w);
  }
  calculate_layout(w);
}

// TODO: Rework this code
// All possible inputs should have an associated message
bool handle_input(int socket, char *buf, int read_size) {
  if (read_size == 0) { // Client disconnected
    printf("Client disconnected (%d)\n", socket);
    die(NULL);
    return false;
  } else if (read_size == -1) { // Error
    printf("Error reading from client (%d)\n", socket);
    die(NULL);
    return false;
  } else if (buf[0] == 's') { // Size
    uint32_t width;
    uint32_t height;
    memcpy(&width, buf + 1, sizeof(uint32_t));
    memcpy(&height, buf + 5, sizeof(uint32_t));
    printf("Size (%d): %d, %d\n", socket, width, height);
    Session *session = &neotmux->sessions[neotmux->current_session];
    Window *window = &session->windows[session->current_window];
    window->width = width;
    window->height = height;
    reorder_panes(window);
    dirty = true;
  } else if (buf[0] == 'e') { // Event
    if (read_size > 7) {
      read_size = 7; // TODO: Fix this
    }

    printf("Received (%d):", socket);
    for (int i = 1; i < read_size; i++) {
      if (isprint(buf[i])) {
        printf(" (%d, %c)", buf[i], buf[i]);
      } else {
        printf(" (%d)", buf[i]);
      }
    }
    printf("\n");
    fflush(stdout);

    if (read_size == 7 && buf[1] == '\033' && buf[2] == '[' && buf[3] == 'M') {
      if (handle_mouse(socket, buf, read_size)) {
        dirty = true;
      }
    } else {
      Session *session = &neotmux->sessions[neotmux->current_session];
      Window *w = &session->windows[session->current_window];
      Pane *p = &w->panes[w->current_pane];
      int fd = p->process.fd;
      write(fd, buf + 1, read_size - 1);
    }
  } else if (buf[0] == 'c') { // Command
    handle_command(socket, buf, read_size);
    dirty = true;
  } else {
    printf("Unhandled input (%d)\n", socket);
    fflush(stdout);
  }

  return true;
}

void *handle_client(void *socket_desc) {
  int socket = *(int *)socket_desc;
  int frame = 0;

  printf("Client connected (%d)\n", socket);

  handle_input(socket, "cInit", 5);

  while (1) {
    int sixty_fps = 1000000 / 60;
    struct timeval tv = {.tv_sec = 0, .tv_usec = sixty_fps};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket, &fds);
    int max_fd = socket;

    pthread_mutex_lock(&neotmux->mutex);
    for (int i = 0; i < neotmux->session_count; i++) {
      Session *session = &neotmux->sessions[i];
      if (session[i].current_window < 0) {
        continue;
      }
      // TODO: Should be pointers
      Window *w = &session[i].windows[session[i].current_window];
      for (int j = 0; j < w->pane_count; j++) {
        Pane *p = &w->panes[j];
        if (p->process.closed) {
          continue;
        }

        FD_SET(p->process.fd, &fds);
        if (p->process.fd > max_fd) {
          max_fd = p->process.fd;
        }
      }
    }
    pthread_mutex_unlock(&neotmux->mutex);

    int retval = select(max_fd + 1, &fds, NULL, NULL, &tv);
    if (retval == -1) {
      perror("select()");
      break;
    }

    pthread_mutex_lock(&neotmux->mutex);
    for (int i = 0; i < neotmux->session_count; i++) {
      Session *session = &neotmux->sessions[i];
      if (session[i].current_window < 0) {
        continue;
      }
      Window *w = &session[i].windows[session[i].current_window];
      for (int j = 0; j < w->pane_count; j++) {
        Pane *p = &w->panes[j];
        if (FD_ISSET(p->process.fd, &fds)) {
          if (p->process.closed) {
            continue;
          }

          char buf[32];
          int read_size = read(p->process.fd, buf, 32);
          if (read_size == 0) {
            printf("Process disconnected (%d)\n", p->process.fd);
            fflush(stdout);
            exit(EXIT_FAILURE);
          } else if (read_size == -1) {
            p->process.closed = true;
            printf("Process closed (%d)\n", p->process.fd);
            reorder_panes(w);
            continue;
          }
          vterm_input_write(p->process.vt, buf, read_size);
          dirty = true;
        }
      }
    }
    pthread_mutex_unlock(&neotmux->mutex);

    pthread_mutex_lock(&neotmux->mutex);
    if (retval && FD_ISSET(socket, &fds)) {
      char buf[32];
      int read_size = read_message(socket, buf, 32);
      if (!handle_input(socket, buf, read_size)) {
        break;
      }
    } else if (retval == 0) { // Timeout
      if (dirty) {
        Session *session = &neotmux->sessions[neotmux->current_session];
        if (session->window_count == 0) {
          send(socket, "e", 1, 0);
          die("No windows");
        }
        Window *w = &session->windows[session->current_window];
        render_screen(socket, w->height, w->width);
        render_bar(socket, w->height, w->width);
        dirty = false;
      }
      if (frame % 60 == 0) {
        Session *session = &neotmux->sessions[neotmux->current_session];
        Window *w = &session->windows[session->current_window];
        render_bar(socket, w->height, w->width);
      }
      frame++;
    }
    pthread_mutex_unlock(&neotmux->mutex);
  }

  free(socket_desc);
  return 0;
}

int start_server(int port) {
  neotmux = malloc(sizeof(*neotmux));
  neotmux->sessions = NULL;
  neotmux->session_count = 0;
  neotmux->current_session = 0;
  neotmux->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  bzero(&neotmux->bb, sizeof(neotmux->bb));
  bzero(&neotmux->prevCell, sizeof(neotmux->prevCell));
  neotmux->barPos = BAR_NONE;
  neotmux->statusBarIdx = 0;
  neotmux->lua = luaL_newstate();
  luaL_openlibs(neotmux->lua); // What does this do?
  char *lua = "print('Lua initialized')";
  luaL_dostring(neotmux->lua, lua);

  char *home = getenv("HOME");
  char dotfile[PATH_MAX];
  snprintf(dotfile, PATH_MAX, "%s/.ntmux.lua", home);

  if (!execute_lua_file(neotmux->lua, "lua/default.lua")) {
    return EXIT_FAILURE;
  }

  load_plugins(neotmux->lua);

  if (!execute_lua_file(neotmux->lua, dotfile)) {
    return EXIT_FAILURE;
  }

  if (!execute_lua_file(neotmux->lua, "lua/init.lua")) {
    return EXIT_FAILURE;
  }

  signal(SIGINT, handle_ctrl_c);

  int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  die_socket_desc = socket_desc;
  if (socket_desc == -1) {
    printf("Could not create socket");
    return EXIT_FAILURE;
  }

  setup_server(socket_desc, port);
  while (1) {
    wait_for_connections(socket_desc);
    struct sockaddr_in client;
    int *socket = malloc(sizeof(*socket));
    *socket = accept_connection(socket_desc, client);

    pthread_t t;
    if (pthread_create(&t, NULL, handle_client, (void *)socket) < 0) {
      perror("Could not create thread");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
