#define _GNU_SOURCE

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#include "command.h"
#include "connection.h"
#include "log.h"
#include "mouse.h"
#include "session.h"

bool dirty = true; // TODO: Dirty should be on a per-pane basis
Neotmux *neotmux;

void run_command(int socket, char *buf, int read_size) {
  handle_command(socket, buf, read_size);
}

// Read a u32 representing the size of the message followed by the message
ssize_t read_message(int sock, char *buf, size_t len) {
  // call_function(neotmux->lua, "helloWorld");
  uint32_t size;
  if (read(sock, &size, sizeof(uint32_t)) != sizeof(uint32_t)) {
    return -1;
  }

  if (size > len) {
    return -1;
  }

  return read(sock, buf, size);
}

int die_socket_desc = -1;
#define die()                                                                  \
  close(die_socket_desc);                                                      \
  WRITE_LOG(LOG_INFO, socket, "Exiting...");                                   \
  exit(EXIT_SUCCESS);

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

bool handle_input(int socket, char *buf, int read_size);

void reorder_panes(int socket, Window *w) {
  for (int i = 0; i < w->pane_count; i++) {
    Pane *p = &w->panes[i];
    if (p->process->closed) {
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

  handle_input(socket, "cLayout", 7);
}

// TODO: Rework this code
// All possible inputs should have an associated message
bool handle_input(int socket, char *buf, int read_size) {
  if (read_size == 0) { // Client disconnected
    WRITE_LOG(LOG_INFO, socket, "Client disconnected");
    return false;
  } else if (read_size == -1) { // Error
    WRITE_LOG(LOG_INFO, socket, "Error reading from client");
    return false;
  } else if (buf[0] == 's') { // Size
    uint32_t width;
    uint32_t height;
    memcpy(&width, buf + 1, sizeof(uint32_t));
    memcpy(&height, buf + 5, sizeof(uint32_t));
    WRITE_LOG(LOG_INFO, socket, "Size %dx%d", width, height);
    Window *window = get_current_window(neotmux);
    window->width = width;
    window->height = height;
    reorder_panes(socket, window);
    dirty = true;
  } else if (buf[0] == 'e') { // Event
    for (int i = 1; i < read_size; i++) {
      if (isprint(buf[i])) {
        WRITE_LOG(LOG_EVENT, socket, "Event (%d, %c)", buf[i], buf[i]);
      } else {
        WRITE_LOG(LOG_EVENT, socket, "Event (%d)", buf[i]);
      }
    }

    // TODO: Fix this
    if (read_size == 7 && buf[1] == '\033' && buf[2] == '[' && buf[3] == 'M') {
      if (handle_mouse(socket, buf, read_size)) {
        dirty = true;
      }
    } else {
      Pane *p = get_current_pane(neotmux);
      write(p->process->fd, buf + 1, read_size - 1);
    }
  } else if (buf[0] == 'c') { // Command
    run_command(socket, buf, read_size);
    dirty = true;
  } else {
    WRITE_LOG(LOG_WARN, socket, "Unhandled input");
  }

  return true;
}

// TODO: Investigate this
bool check_timeout(int timeout) {
  static struct timeval tv = {.tv_sec = 0, .tv_usec = 0};

  struct timeval now;
  gettimeofday(&now, NULL);

  if (tv.tv_sec == 0 && tv.tv_usec == 0) {
    tv = now;
    return true;
  }

  if (now.tv_sec - tv.tv_sec > 0) {
    tv = now;
    return true;
  }

  if (now.tv_usec - tv.tv_usec > timeout) {
    tv = now;
    return true;
  }

  return false;
}

void *handle_client(void *socket_desc) {
  int socket = *(int *)socket_desc;
  int frame = 0;

  WRITE_LOG(LOG_INFO, socket, "Client connected");

  handle_input(socket, "cInit", 5);

  while (1) {
    int sixty_fps = 1000000 / 60;
    struct timeval tv = {.tv_sec = 0, .tv_usec = sixty_fps};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket, &fds);
    int max_fd = socket;

    // pthread_mutex_lock(&neotmux->mutex);
    for (int i = 0; i < neotmux->session_count; i++) {
      Session *session = &neotmux->sessions[i];
      if (session[i].current_window < 0) {
        continue;
      }
      Window *w = &session[i].windows[session[i].current_window];
      for (int j = 0; j < w->pane_count; j++) {
        Pane *p = &w->panes[j];
        if (p->process->closed) {
          continue;
        }

        FD_SET(p->process->fd, &fds);
        if (p->process->fd > max_fd) {
          max_fd = p->process->fd;
        }
      }
    }

    int retval = select(max_fd + 1, &fds, NULL, NULL, &tv);
    if (retval == -1) {
      perror("select()");
      break;
    }

    for (int i = 0; i < neotmux->session_count; i++) {
      Session *session = &neotmux->sessions[i];
      if (session[i].current_window < 0) {
        continue;
      }
      Window *w = &session[i].windows[session[i].current_window];
      for (int j = 0; j < w->pane_count; j++) {
        Pane *p = &w->panes[j];
        if (FD_ISSET(p->process->fd, &fds)) {
          if (p->process->closed) {
            continue;
          }

          // TODO: Tune this number
          static char buf[32000];
          int read_size = read(p->process->fd, buf, 32000);
          if (read_size == 0) {
            WRITE_LOG(LOG_INFO, socket, "Process disconnected (%d)",
                      p->process->fd);
            exit(EXIT_FAILURE);
          } else if (read_size == -1) {
            p->process->closed = true;
            WRITE_LOG(LOG_INFO, socket, "Process closed (FD: %d PID: %d)",
                      p->process->fd, p->process->pid);
            reorder_panes(socket, w);
            run_command(socket, "cRenderBar", 10);
            run_command(socket, "cRenderScreen", 13);
            continue;
          }
          vterm_input_write(p->process->vt, buf, read_size);
          dirty = true;
        }
      }
    }

    if (retval && FD_ISSET(socket, &fds)) {
      char buf[32];
      int read_size = read_message(socket, buf, 32);
      if (!handle_input(socket, buf, read_size)) {
        break;
      }
    } else if (retval == 0) { // Timeout
    }

    if (dirty && check_timeout(1000000 / 60)) {
      Session *session = get_current_session(neotmux);
      if (session->window_count == 0) {
        send(socket, "", 0, 0);
        WRITE_LOG(LOG_INFO, socket, "No windows");
        die();
      }
      run_command(socket, "cRenderScreen", 13);
      dirty = false;
    }

    if (frame % 60 == 0) {
      run_command(socket, "cRenderBar", 10);
    }
    frame++;
    // pthread_mutex_unlock(&neotmux->mutex);
  }

  free(socket_desc);
  return 0;
}

int init_ntmux(char *log_filename, char **commands, int nCommands) {
  neotmux = malloc(sizeof(*neotmux));
  neotmux->log = fopen(log_filename, "w+");
  neotmux->sessions = NULL;
  neotmux->session_count = 0;
  neotmux->current_session = 0;
  neotmux->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  bzero(&neotmux->bb, sizeof(neotmux->bb));
  bzero(&neotmux->prevCell, sizeof(neotmux->prevCell));
  neotmux->barPos = BAR_NONE;
  neotmux->statusBarIdx = 0;
  neotmux->commands = commands;
  neotmux->nCommands = nCommands;

  return EXIT_SUCCESS;
}

char *sockname;
void handle_cleanup() {
  struct sockaddr_un server;
  snprintf(server.sun_path, sizeof(server.sun_path), "/tmp/ntmux-1000/%s.sock",
           sockname);
  unlink(server.sun_path);
  fclose(neotmux->log);
}

void start_server_loop(int socket_desc, char *log_filename, char **commands,
                       int nCommands) {
  die_socket_desc = socket_desc;

  atexit(handle_cleanup);

  if (init_ntmux(log_filename, commands, nCommands) == EXIT_FAILURE) {
    return;
  }

  while (1) {
    int *socket = init_connection(socket_desc);

    pthread_t t;
    if (pthread_create(&t, NULL, handle_client, (void *)socket) < 0) {
      perror("Could not create thread");
      break;
    }
  }
}

void handle_ctrl_c() { signal(SIGINT, handle_ctrl_c); }

int start_server(int sock, char *name, char *log_filename, char **commands,
                 int nCommands) {
  signal(SIGINT, handle_ctrl_c);

  sockname = name;

  start_server_loop(sock, log_filename, commands, nCommands);

  return EXIT_SUCCESS;
}
