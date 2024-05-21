#define _GNU_SOURCE

#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>

#include "log.h"
#include "print_session.h"

extern Neotmux *neotmux;

#define LOG(fmt, ...) fprintf(neotmux->log, fmt, ##__VA_ARGS__)

void print_selection(Selection *selection) {
  LOG("      Selection: %s\n", selection->active ? "Active" : "Inactive");
  LOG("        Start: %d, %d\n", selection->start.col, selection->start.row);
  LOG("        End: %d, %d\n", selection->end.col, selection->end.row);
}

void print_cursor_shape(int cursor_shape) {
  switch (cursor_shape) {
  case 0:
    LOG("          Shape: Default\n");
    break;
  case VTERM_PROP_CURSORSHAPE_BLOCK:
    LOG("          Shape: Block\n");
    break;
  case VTERM_PROP_CURSORSHAPE_UNDERLINE:
    LOG("          Shape: Underline\n");
    break;
  case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
    LOG("          Shape: Bar Left\n");
    break;
  default:
    LOG("          Shape: Unknown\n");
    break;
  }
}

void print_cursor(Cursor *cursor) {
  LOG("        Cursor: %s\n", cursor->mouse_active ? "Active" : "Inactive");
  LOG("          Visible: %s\n", cursor->visible ? "True" : "False");
  print_cursor_shape(cursor->shape);
}

void print_scroll_back_lines(ScrollBackLines *scrollback) {
  LOG("        Scrollback: %zu/%zu\n", scrollback->size, scrollback->capacity);
}

void print_cwd(pid_t pid) {
  char cwd[PATH_MAX] = {0};
  char link[PATH_MAX] = {0};
  sprintf(link, "/proc/%d/cwd", pid);
  if (readlink(link, cwd, sizeof(cwd)) != -1) {
    LOG("        CWD: %s\n", cwd);
  }
}

void print_process(Process *process) {
  if (process->pid != -1) {
    LOG("      Process: %s\n", process->name);
    LOG("        PID: %d\n", process->pid);
    LOG("        FD: %d\n", process->fd);
    LOG("        Closed: %s\n", process->closed ? "True" : "False");
    LOG("        Scrolloffset: %d\n", process->scrolloffset);
    print_scroll_back_lines(&process->scrollback);
    print_cwd(process->pid);
    print_cursor(&process->cursor);
  } else {
    LOG("      Process: %s\n", "None");
  }
}

void print_pane(Pane *pane) {
  LOG("    Pane: %p\n", (void *)pane);
  LOG("      Col: %d\n", pane->col);
  LOG("      Row: %d\n", pane->row);
  LOG("      Width: %d\n", pane->width);
  LOG("      Height: %d\n", pane->height);

  print_selection(&pane->selection);
  print_process(pane->process);
}


void print_window(Window *window) {
  LOG("  Window: %s\n", window->title);
  LOG("    Width: %d\n", window->width);
  LOG("    Height: %d\n", window->height);
  if (window->zoom == -1) {
    LOG("    Zoom: None\n");
  } else {
    LOG("    Zoom: %d\n", window->zoom);
  }
  LOG("    Current Pane: %d\n", window->current_pane);
  LOG("    %s\n", window->layout);

  for (int i = 0; i < window->pane_count; i++) {
    print_pane(&window->panes[i]);
  }
}

void print_session(Session *session) {
  LOG("Session: %s\n", session->title);
  LOG(" Current Window: %d\n", session->current_window);
  for (int i = 0; i < session->window_count; i++) {
    print_window(&session->windows[i]);
  }
}

void print_sessions(Neotmux *neotmux, int socket) {
  WRITE_LOG(LOG_INFO, socket, "Printing Sessions");

  int count = neotmux->session_count;
  Session *sessions = neotmux->sessions;
  for (int i = 0; i < count; i++) {
    print_session(&sessions[i]);
  }
}
