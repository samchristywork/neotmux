#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <vterm.h>

#include "pty.h"
#include "render.h"
#include "session.h"

Session *sessions = NULL;
int num_sessions = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_NAME 1024

void init_screen(VTerm **vt, VTermScreen **vts, int h, int w) {
  *vt = vterm_new(h, w);
  if (!vt) {
    exit(EXIT_FAILURE);
  }

  *vts = vterm_obtain_screen(*vt);

  // TODO: Implement callbacks
  static VTermScreenCallbacks callbacks;
  callbacks.damage = NULL;
  callbacks.moverect = NULL;
  callbacks.movecursor = NULL;
  callbacks.settermprop = NULL;
  callbacks.bell = NULL;
  callbacks.resize = NULL;
  callbacks.sb_pushline = NULL;
  callbacks.sb_popline = NULL;

  vterm_set_utf8(*vt, 1);
  vterm_screen_reset(*vts, 1);
  vterm_screen_enable_altscreen(*vts, 1);
  vterm_screen_set_callbacks(*vts, &callbacks, NULL);
}

void add_process_to_pane(Pane *pane) {
  struct winsize ws;
  ws.ws_col = 80;
  ws.ws_row = 12;
  ws.ws_xpixel = 0;
  ws.ws_ypixel = 0;

  char childName[MAX_NAME];
  pid_t childPid = pty_fork(&pane->process.fd, childName, MAX_NAME, &ws);
  if (childPid == -1) {
    exit(EXIT_FAILURE);
  }

  if (childPid == 0) { // Child
    char *shell = getenv("SHELL");
    execlp(shell, shell, (char *)NULL);
    exit(EXIT_FAILURE);
  }

  pane->process.pid = childPid;
  pane->process.name = malloc(strlen(childName) + 1);
  strcpy(pane->process.name, childName);

  init_screen(&pane->process.vt, &pane->process.vts, ws.ws_row, ws.ws_col);
}

void initialize_session() {
  sessions = malloc(sizeof(*sessions));
  sessions->title = "Main Session";
  sessions->window_count = 0;
  sessions->current_window = 0;
  num_sessions++;

  Window *window = add_window(sessions, "Main Window");
  add_pane(window, 0, 0, 80, 12);

  Pane *pane = &window->panes[0];
  add_process_to_pane(pane);

  print_sessions(sessions, num_sessions);
}

struct sockaddr_in setup_server(int socket_desc) {
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(8888);

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

bool handle_input(int socket, char *buf, int read_size) {
  if (read_size == 0) {
    printf("Client disconnected (%d)\n", socket);
    fflush(stdout);
    return false;
  } else if (read_size == -1) {
    perror("recv failed");
    return false;
  } else if (buf[0] == 'e') {
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

    pthread_mutex_lock(&mutex);
    Window w = sessions->windows[sessions->current_window];
    Pane p = w.panes[w.current_pane];
    int fd = p.process.fd;
    write(fd, buf + 1, read_size - 1);
    pthread_mutex_unlock(&mutex);
  } else if (buf[0] == 'c') {
    char commandString[read_size];
    strncpy(commandString, buf + 1, read_size - 1);

    if (strcmp(commandString, "Create") == 0) {
      pthread_mutex_lock(&mutex);
      add_window(sessions, "New Window");
      print_sessions(sessions, num_sessions);
      pthread_mutex_unlock(&mutex);
    } else if (strcmp(commandString, "List") == 0) {
      pthread_mutex_lock(&mutex);
      print_sessions(sessions, num_sessions);
      pthread_mutex_unlock(&mutex);
    } else if (strcmp(commandString, "Reload") == 0) {
      printf("Reloading (%d)\n", socket);
      fflush(stdout);
      exit(EXIT_SUCCESS);
    } else {
      printf("Unhandled command (%d): %s\n", socket, commandString);
      fflush(stdout);
    }
  } else {
    printf("Unhandled input (%d)\n", socket);
    fflush(stdout);
  }

  return true;
}

void *client_handler(void *socket_desc) {
  int socket = *(int *)socket_desc;
  int frame = 0;

  printf("Client connected (%d)\n", socket);

  bool dirty = true;
  while (1) {
    int sixty_fps = 1000000 / 60;
    struct timeval tv = {.tv_sec = 0, .tv_usec = sixty_fps};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket, &fds);
    int max_fd = socket;

    for (int i = 0; i < num_sessions; i++) {
      Window w = sessions[i].windows[sessions[i].current_window];
      for (int j = 0; j < w.pane_count; j++) {
        Pane p = w.panes[j];
        if (p.process.closed) {
          continue;
        }

        FD_SET(p.process.fd, &fds);
        if (p.process.fd > max_fd) {
          max_fd = p.process.fd;
        }
      }
    }

    int retval = select(max_fd + 1, &fds, NULL, NULL, &tv);
    if (retval == -1) {
      perror("select()");
      break;
    }

    for (int i = 0; i < num_sessions; i++) {
      Window *w = &sessions[i].windows[sessions[i].current_window];
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
            continue;
          }
          // write(socket, buf, read_size);
          vterm_input_write(p->process.vt, buf, read_size);
          dirty = true;
        }
      }
    }

    if (retval && FD_ISSET(socket, &fds)) {
      char buf[32];
      int read_size = recv(socket, buf, 32, 0);
      if (!handle_input(socket, buf, read_size)) {
        break;
      }
    } else if (retval == 0) { // Timeout
      if (dirty) {
        render_screen(socket);
        dirty = false;
      }
    }
  }

  free(socket_desc);
  return 0;
}

int server() {
  initialize_session();

  int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Could not create socket");
    return EXIT_FAILURE;
  }

  struct sockaddr_in server = setup_server(socket_desc);
  while (1) {
    wait_for_connections(socket_desc);
    struct sockaddr_in client;
    int *socket = malloc(sizeof(*socket));
    *socket = accept_connection(socket_desc, client);

    pthread_t t;
    if (pthread_create(&t, NULL, client_handler, (void *)socket) < 0) {
      perror("Could not create thread");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
