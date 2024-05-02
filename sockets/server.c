#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <vterm.h>

#include "layout.h"
#include "pty.h"
#include "render.h"
#include "session.h"

bool dirty = true; // TODO: Dirty should be on a per-pane basis
Neotmux *neotmux;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_NAME 1024

int ctrl_c_socket_desc;
void ctrl_c(int sig) {
  signal(SIGINT, ctrl_c);
  close(ctrl_c_socket_desc);
  printf("\nExiting...\n");
  exit(EXIT_SUCCESS);
}

int term_prop_callback(VTermProp prop, VTermValue *val, void *user) {
  Pane *pane = (Pane *)user;
  if (prop == VTERM_PROP_CURSORVISIBLE) {
    pane->process.cursor_visible = val->boolean;
  } else if (prop == VTERM_PROP_CURSORSHAPE) {
    pane->process.cursor_shape = val->number;
  } else if (prop == VTERM_PROP_MOUSE) {
    pane->process.mouse = val->number;
  }

  return 0;
}

void init_screen(VTerm **vt, VTermScreen **vts, int h, int w, Pane *pane) {
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
  callbacks.settermprop = term_prop_callback;
  callbacks.bell = NULL;
  callbacks.resize = NULL;
  callbacks.sb_pushline = NULL;
  callbacks.sb_popline = NULL;

  vterm_set_utf8(*vt, 1);
  vterm_screen_reset(*vts, 1);
  vterm_screen_enable_altscreen(*vts, 1);
  vterm_screen_set_callbacks(*vts, &callbacks, pane);
}

void add_process_to_pane(Pane *pane) {
  struct winsize ws;
  ws.ws_col = pane->width;
  ws.ws_row = pane->height;
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

  init_screen(&pane->process.vt, &pane->process.vts, ws.ws_row, ws.ws_col,
              pane);
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

bool within_rect(int x, int y, int x1, int y1, int w, int h) {
  return x >= x1 && x < x1 + w && y >= y1 && y < y1 + h;
}

int left(Window *w) {
  Pane *currentPane = &w->panes[w->current_pane];
  int col = currentPane->col;
  int row = currentPane->row;
  col--;

  while (true) {
    if (col < 0) {
      col = w->width - 1;
    }
    for (int i = 0; i < w->pane_count; i++) {
      Pane *pane = &w->panes[i];
      if (within_rect(col, row, pane->col, pane->row, pane->width,
                      pane->height)) {
        return i;
      }
    }
    col--;
  }
}

int right(Window *w) {
  Pane *currentPane = &w->panes[w->current_pane];
  int col = currentPane->col;
  int row = currentPane->row;
  col += currentPane->width;

  while (true) {
    if (col >= w->width) {
      col = 0;
    }
    for (int i = 0; i < w->pane_count; i++) {
      Pane *pane = &w->panes[i];
      if (within_rect(col, row, pane->col, pane->row, pane->width,
                      pane->height)) {
        return i;
      }
    }
    col++;
  }
}

int up(Window *w) {
  Pane *currentPane = &w->panes[w->current_pane];
  int col = currentPane->col;
  int row = currentPane->row;
  row--;

  while (true) {
    if (row < 0) {
      row = w->height - 1;
    }
    for (int i = 0; i < w->pane_count; i++) {
      Pane *pane = &w->panes[i];
      if (within_rect(col, row, pane->col, pane->row, pane->width,
                      pane->height)) {
        return i;
      }
    }
    row--;
  }
}

int down(Window *w) {
  Pane *currentPane = &w->panes[w->current_pane];
  int col = currentPane->col;
  int row = currentPane->row;
  row += currentPane->height;

  while (true) {
    if (row >= w->height) {
      row = 0;
    }
    for (int i = 0; i < w->pane_count; i++) {
      Pane *pane = &w->panes[i];
      if (within_rect(col, row, pane->col, pane->row, pane->width,
                      pane->height)) {
        return i;
      }
    }
    row++;
  }
}

// TODO: This should happen on the client side
char *get_user_input(int socket, char *prompt) {
  write(socket, "NTMUX_GET_INPUT", 15);
  // char buf[1024];
  // int read_size = recv(socket, buf, 1024, 0);
  // if (read_size == 0) {
  //   printf("Client disconnected (%d)\n", socket);
  //   fflush(stdout);
  //   return NULL;
  // } else if (read_size == -1) {
  //   perror("recv failed");
  //   return NULL;
  // } else {
  //   char *ret = malloc(read_size+1);
  //   memcpy(ret, buf, read_size);
  //   ret[read_size] = '\0';
  //   return ret;
  // }
  // return NULL;

  // char input[1024];
  // int idx = 0;

  // Session *session = &neotmux->sessions[neotmux->current_session];
  // Window *window = &session->windows[session->current_window];
  // char buf[32];
  // int n = snprintf(buf, 32, "\033[%d;%dH", window->height + 1, 1);
  // write(socket, buf, n);

  // char *clear_line = "\033[2K";
  // write(socket, clear_line, strlen(clear_line));

  // send(socket, prompt, strlen(prompt), 0);
  // while (true) {
  //   char buf[2];
  //   int read_size = recv(socket, buf, 2, 0);
  //   if (read_size == 0) {
  //     printf("Client disconnected (%d)\n", socket);
  //     fflush(stdout);
  //     return NULL;
  //   } else if (read_size == -1) {
  //     perror("recv failed");
  //     return NULL;
  //   } else if (read_size == 2 && buf[0] == 'e') {
  //     if (buf[1] == '\n') {
  //       input[idx] = '\0';
  //       return strdup(input);
  //     } else {
  //       input[idx++] = buf[1];
  //       send(socket, buf + 1, 1, 0);
  //     }
  //   }
  // }
}

// TODO: Rework this code
// All possible inputs should have an associated message
bool handle_input(int socket, char *buf, int read_size) {
  if (read_size == 0) { // Client disconnected
    printf("Client disconnected (%d)\n", socket);
    fflush(stdout);
    return false;
  } else if (read_size == -1) { // Error
    perror("recv failed");
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
    calculate_layout(window);
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
      printf("Mouse event (%d): ", socket);

      // Must be unsigned
      int b = (unsigned char)buf[4] & 0x03;
      int x = (unsigned char)buf[5] - 0x20;
      int y = (unsigned char)buf[6] - 0x20;

      printf("%d, %d, %d\n", x, y, b);
      fflush(stdout);

      if (b == 0) {
        printf("Left click\n");
        Session *session = &neotmux->sessions[neotmux->current_session];
        Window *w = &session->windows[session->current_window];
        for (int i = 0; i < w->pane_count; i++) {
          printf("Checking pane %d\n", i);
          printf("  %d, %d, %d, %d\n", w->panes[i].col, w->panes[i].row,
                 w->panes[i].width, w->panes[i].height);
          Pane *p = &w->panes[i];
          if (within_rect(x - 1, y - 1, p->col, p->row, p->width, p->height)) {
            printf("Found pane %d\n", i);
            w->current_pane = i;
            calculate_layout(w);
            dirty = true;
            break;
          }
        }
      }

      Session *session = &neotmux->sessions[neotmux->current_session];
      Window *window = &session->windows[session->current_window];
      Pane *pane = &window->panes[window->current_pane];

      // VTERM_PROP_MOUSE_NONE = 0,
      // VTERM_PROP_MOUSE_CLICK,
      // VTERM_PROP_MOUSE_DRAG,
      // VTERM_PROP_MOUSE_MOVE,
      // TODO: Handle all mouse states
      if (pane->process.mouse != VTERM_PROP_MOUSE_NONE) {
        pthread_mutex_lock(&mutex);
        write(pane->process.fd, buf + 1, read_size - 1);
        // TODO: This triggers a redraw, which we might not want.
        pthread_mutex_unlock(&mutex);
      }
    } else {
      pthread_mutex_lock(&mutex);
      Session *session = &neotmux->sessions[neotmux->current_session];
      Window w = session->windows[session->current_window];
      Pane p = w.panes[w.current_pane];
      int fd = p.process.fd;
      write(fd, buf + 1, read_size - 1);
      pthread_mutex_unlock(&mutex);
    }
  } else if (buf[0] == 'c') { // Command
    char cmd[read_size];
    memcpy(cmd, buf + 1, read_size - 1);
    cmd[read_size - 1] = '\0';

    pthread_mutex_lock(&mutex);
    Session *session = &neotmux->sessions[neotmux->current_session];
    Window *w = &session->windows[session->current_window];

    if (strcmp(cmd, "Create") == 0) {
      Session *session = &neotmux->sessions[neotmux->current_session];
      Window *window = add_window(session, "New Window");
      add_pane(window, 0, 0, window->width, window->height);
      add_process_to_pane(&window->panes[0]);
      calculate_layout(window);
      dirty = true;
    } else if (strcmp(cmd, "VSplit") == 0) {
      Pane *currentPane = &w->panes[w->current_pane];
      int col = currentPane->col;
      int row = currentPane->row;
      int width = currentPane->width;
      int height = currentPane->height;

      Pane *newPane = add_pane(w, col, row, width, height);
      add_process_to_pane(newPane);
      calculate_layout(w);
      w->current_pane = w->pane_count - 1;
      dirty = true;
    } else if (strcmp(cmd, "Split") == 0) {
      Pane *currentPane = &w->panes[w->current_pane];
      int col = currentPane->col;
      int row = currentPane->row;
      int width = currentPane->width;
      int height = currentPane->height;

      Pane *newPane = add_pane(w, col, row, width, height);
      add_process_to_pane(newPane);
      calculate_layout(w);
      w->current_pane = w->pane_count - 1;
      dirty = true;
    } else if (strcmp(cmd, "List") == 0) {
      print_sessions(neotmux);
    } else if (strcmp(cmd, "Next") == 0) {
      Session *session = &neotmux->sessions[neotmux->current_session];
      session->current_window++;
      if (session->current_window >= session->window_count) {
        session->current_window = 0;
      }
      calculate_layout(&session->windows[session->current_window]);
      dirty = true;
    } else if (strcmp(cmd, "Prev") == 0) {
      Session *session = &neotmux->sessions[neotmux->current_session];
      session->current_window--;
      if (session->current_window < 0) {
        session->current_window = session->window_count - 1;
      }
      calculate_layout(&session->windows[session->current_window]);
      dirty = true;
    } else if (strcmp(cmd, "Left") == 0) {
      w->current_pane = left(w);
      dirty = true;
    } else if (strcmp(cmd, "Right") == 0) {
      w->current_pane = right(w);
      dirty = true;
    } else if (strcmp(cmd, "Up") == 0) {
      w->current_pane = up(w);
      dirty = true;
    } else if (strcmp(cmd, "Down") == 0) {
      w->current_pane = down(w);
      dirty = true;
    } else if (strcmp(cmd, "Even_Horizontal") == 0) {
      printf("Even Horizontal\n");
      w->layout = LAYOUT_EVEN_HORIZONTAL;
      calculate_layout(w);
      dirty = true;
    } else if (strcmp(cmd, "Even_Vertical") == 0) {
      printf("Even Vertical\n");
      w->layout = LAYOUT_EVEN_VERTICAL;
      calculate_layout(w);
      dirty = true;
    } else if (strcmp(cmd, "Main_Horizontal") == 0) {
      printf("Main Horizontal\n");
      w->layout = LAYOUT_MAIN_HORIZONTAL;
      calculate_layout(w);
      dirty = true;
    } else if (strcmp(cmd, "Main_Vertical") == 0) {
      printf("Main Vertical\n");
      w->layout = LAYOUT_MAIN_VERTICAL;
      calculate_layout(w);
      dirty = true;
    } else if (strcmp(cmd, "Tiled") == 0) {
      printf("Tiled\n");
      w->layout = LAYOUT_TILED;
      calculate_layout(w);
      dirty = true;
    } else if (strcmp(cmd, "Zoom") == 0) {
      if (w->zoom == -1) {
        w->zoom = w->current_pane;
      } else {
        w->zoom = -1;
      }
      calculate_layout(w);
      dirty = true;
    } else if (memcmp(cmd, "RenameWindow", 12) == 0) {
      printf("Rename Window\n");
      Window *window = &session->windows[session->current_window];
      free(window->title);
      char *title = strdup(cmd + 13);
      window->title = title;
      dirty = true;
    } else if (memcmp(cmd, "RenameSession", 13) == 0) {
      printf("Rename Session\n");
      Session *session = &neotmux->sessions[neotmux->current_session];
      free(session->title);
      char *title = strdup(cmd + 14);
      session->title = title;
      dirty = true;
    } else if (strcmp(cmd, "Reload") == 0) {
      printf("Reloading (%d)\n", socket);
      fflush(stdout);
      pthread_mutex_unlock(&mutex);
      close(socket);
      exit(EXIT_SUCCESS);
    } else {
      printf("Unhandled command (%d): %s\n", socket, cmd);
      fflush(stdout);
    }
    pthread_mutex_unlock(&mutex);
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
        Window *w = &sessions->windows[sessions->current_window];
        render_screen(socket, w->height, w->width);
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
