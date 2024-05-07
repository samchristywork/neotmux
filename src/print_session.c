#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>

#include "print_session.h"

extern Neotmux *neotmux;

void print_layout(Layout layout) {
  switch (layout) {
  case LAYOUT_DEFAULT:
    fprintf(neotmux->log, "Default");
    break;
  case LAYOUT_EVEN_HORIZONTAL:
    fprintf(neotmux->log, "Even Horizontal");
    break;
  case LAYOUT_EVEN_VERTICAL:
    fprintf(neotmux->log, "Even Vertical");
    break;
  case LAYOUT_MAIN_HORIZONTAL:
    fprintf(neotmux->log, "Main Horizontal");
    break;
  case LAYOUT_MAIN_VERTICAL:
    fprintf(neotmux->log, "Main Vertical");
    break;
  case LAYOUT_TILED:
    fprintf(neotmux->log, "Tiled");
    break;
  default:
    fprintf(neotmux->log, "Unknown");
    break;
  }
}

void print_cursor_shape(int cursor_shape) {
  switch (cursor_shape) {
  case 0:
    fprintf(neotmux->log, "Default");
    break;
  case VTERM_PROP_CURSORSHAPE_BLOCK:
    fprintf(neotmux->log, "Block");
    break;
  case VTERM_PROP_CURSORSHAPE_UNDERLINE:
    fprintf(neotmux->log, "Underline");
    break;
  case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
    fprintf(neotmux->log, "Bar Left");
    break;
  default:
    fprintf(neotmux->log, "Unknown");
    break;
  }

  fprintf(neotmux->log, "\n");
}

void print_process_cwd(pid_t pid) {
  char cwd[PATH_MAX] = {0};
  char link[PATH_MAX] = {0};
  sprintf(link, "/proc/%d/cwd", pid);
  if (readlink(link, cwd, sizeof(cwd)) != -1) {
    fprintf(neotmux->log, "        cwd: %s\n", cwd);
  }
}

void print_cursor(Cursor *cursor) {
  fprintf(neotmux->log, "          mouse_active: %s\n",
         cursor->mouse_active ? "True" : "False");
  fprintf(neotmux->log, "          shape: ");
  print_cursor_shape(cursor->shape);
  fprintf(neotmux->log, "          visible: %s\n", cursor->visible ? "True" : "False");
}

void print_process(Process *process) {
  if (process->pid != -1) {
    fprintf(neotmux->log, "      Process: %s\n", process->name);
    fprintf(neotmux->log, "        pid: %d\n", process->pid);
    fprintf(neotmux->log, "        fd: %d\n", process->fd);
    fprintf(neotmux->log, "        closed: %s\n", process->closed ? "True" : "False");
    fprintf(neotmux->log, "        cursor:\n");
    print_cursor(&process->cursor);
    print_process_cwd(process->pid);
  } else {
    fprintf(neotmux->log, "      Process: %s\n", "None");
  }
}

void print_pane(Pane *pane) {
  fprintf(neotmux->log, "    Pane: %p\n", (void *)pane);
  fprintf(neotmux->log, "      col: %d\n", pane->col);
  fprintf(neotmux->log, "      row: %d\n", pane->row);
  fprintf(neotmux->log, "      width: %d\n", pane->width);
  fprintf(neotmux->log, "      height: %d\n", pane->height);
  print_process(pane->process);
}

void print_window(Window *window) {
  fprintf(neotmux->log, "  Window: %s\n", window->title);
  fprintf(neotmux->log, "    Pane Count: %d\n", window->pane_count);
  fprintf(neotmux->log, "    Current Pane: %d\n", window->current_pane);
  fprintf(neotmux->log, "    Layout: ");
  print_layout(window->layout);
  fprintf(neotmux->log, "\n");
  for (int j = 0; j < window->pane_count; j++) {
    Pane *pane = &window->panes[j];
    print_pane(pane);
  }
}

void print_session(Session *session) {
  fprintf(neotmux->log, "Session: %s\n", session->title);
  fprintf(neotmux->log, "  Window Count: %d\n", session->window_count);
  fprintf(neotmux->log, "  Current Window: %d\n", session->current_window);
  for (int i = 0; i < session->window_count; i++) {
    Window *window = &session->windows[i];
    print_window(window);
  }
}

void print_sessions(Neotmux *neotmux) {
  int count = neotmux->session_count;
  Session *sessions = neotmux->sessions;
  for (int i = 0; i < count; i++) {
    print_session(&sessions[i]);
  }
}
