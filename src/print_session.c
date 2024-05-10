#define _GNU_SOURCE

#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>

#include "log.h"
#include "print_session.h"

extern Neotmux *neotmux;

// TODO: Make this work with WRITE_LOG
void print_layout(Layout layout, int socket) {
  switch (layout) {
  case LAYOUT_DEFAULT:
    WRITE_LOG(socket, "Default");
    break;
  case LAYOUT_EVEN_HORIZONTAL:
    WRITE_LOG(socket, "Even Horizontal");
    break;
  case LAYOUT_EVEN_VERTICAL:
    WRITE_LOG(socket, "Even Vertical");
    break;
  case LAYOUT_MAIN_HORIZONTAL:
    WRITE_LOG(socket, "Main Horizontal");
    break;
  case LAYOUT_MAIN_VERTICAL:
    WRITE_LOG(socket, "Main Vertical");
    break;
  case LAYOUT_TILED:
    WRITE_LOG(socket, "Tiled");
    break;
  default:
    WRITE_LOG(socket, "Unknown");
    break;
  }
}

void print_cursor_shape(int cursor_shape, int socket) {
  switch (cursor_shape) {
  case 0:
    WRITE_LOG(socket, "Default");
    break;
  case VTERM_PROP_CURSORSHAPE_BLOCK:
    WRITE_LOG(socket, "Block");
    break;
  case VTERM_PROP_CURSORSHAPE_UNDERLINE:
    WRITE_LOG(socket, "Underline");
    break;
  case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
    WRITE_LOG(socket, "Bar Left");
    break;
  default:
    WRITE_LOG(socket, "Unknown");
    break;
  }

  WRITE_LOG(socket, "\n");
}

void print_process_cwd(pid_t pid, int socket) {
  char cwd[PATH_MAX] = {0};
  char link[PATH_MAX] = {0};
  sprintf(link, "/proc/%d/cwd", pid);
  if (readlink(link, cwd, sizeof(cwd)) != -1) {
    WRITE_LOG(socket, "        cwd: %s\n", cwd);
  }
}

void print_cursor(Cursor *cursor, int socket) {
  WRITE_LOG(socket, "          mouse_active: %s\n",
            cursor->mouse_active ? "True" : "False");
  WRITE_LOG(socket, "          shape: ");
  print_cursor_shape(cursor->shape, socket);
  WRITE_LOG(socket, "          visible: %s\n",
            cursor->visible ? "True" : "False");
}

void print_process(Process *process, int socket) {
  if (process->pid != -1) {
    WRITE_LOG(socket, "      Process: %s\n", process->name);
    WRITE_LOG(socket, "        pid: %d\n", process->pid);
    WRITE_LOG(socket, "        fd: %d\n", process->fd);
    WRITE_LOG(socket, "        closed: %s\n",
              process->closed ? "True" : "False");
    WRITE_LOG(socket, "        cursor:\n");
    print_cursor(&process->cursor, socket);
    print_process_cwd(process->pid, socket);
  } else {
    WRITE_LOG(socket, "      Process: %s\n", "None");
  }
}

void print_pane(Pane *pane, int socket) {
  WRITE_LOG(socket, "    Pane: %p\n", (void *)pane);
  WRITE_LOG(socket, "      col: %d\n", pane->col);
  WRITE_LOG(socket, "      row: %d\n", pane->row);
  WRITE_LOG(socket, "      width: %d\n", pane->width);
  WRITE_LOG(socket, "      height: %d\n", pane->height);
  print_process(pane->process, socket);
}

void print_window(Window *window, int socket) {
  WRITE_LOG(socket, "  Window: %s\n", window->title);
  WRITE_LOG(socket, "    Pane Count: %d\n", window->pane_count);
  WRITE_LOG(socket, "    Current Pane: %d\n", window->current_pane);
  WRITE_LOG(socket, "    Layout: ");
  print_layout(window->layout, socket);
  WRITE_LOG(socket, "\n");
  for (int j = 0; j < window->pane_count; j++) {
    Pane *pane = &window->panes[j];
    print_pane(pane, socket);
  }
}

void print_session(Session *session, int socket) {
  WRITE_LOG(socket, "Session: %s\n", session->title);
  WRITE_LOG(socket, "  Window Count: %d\n", session->window_count);
  WRITE_LOG(socket, "  Current Window: %d\n", session->current_window);
  for (int i = 0; i < session->window_count; i++) {
    Window *window = &session->windows[i];
    print_window(window, socket);
  }
}

void print_sessions(Neotmux *neotmux, int socket) {
  int count = neotmux->session_count;
  Session *sessions = neotmux->sessions;
  for (int i = 0; i < count; i++) {
    print_session(&sessions[i], socket);
  }
}
