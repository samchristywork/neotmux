#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>

#include "print_session.h"

void print_layout(Layout layout) {
  switch (layout) {
  case LAYOUT_DEFAULT:
    printf("Default");
    break;
  case LAYOUT_EVEN_HORIZONTAL:
    printf("Even Horizontal");
    break;
  case LAYOUT_EVEN_VERTICAL:
    printf("Even Vertical");
    break;
  case LAYOUT_MAIN_HORIZONTAL:
    printf("Main Horizontal");
    break;
  case LAYOUT_MAIN_VERTICAL:
    printf("Main Vertical");
    break;
  case LAYOUT_TILED:
    printf("Tiled");
    break;
  default:
    printf("Unknown");
    break;
  }
}

void print_cursor_shape(int cursor_shape) {
  switch (cursor_shape) {
  case 0:
    printf("Default");
    break;
  case VTERM_PROP_CURSORSHAPE_BLOCK:
    printf("Block");
    break;
  case VTERM_PROP_CURSORSHAPE_UNDERLINE:
    printf("Underline");
    break;
  case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
    printf("Bar Left");
    break;
  default:
    printf("Unknown");
    break;
  }

  printf("\n");
}

void print_process_cwd(pid_t pid) {
  char cwd[PATH_MAX] = {0};
  char link[PATH_MAX] = {0};
  sprintf(link, "/proc/%d/cwd", pid);
  if (readlink(link, cwd, sizeof(cwd)) != -1) {
    printf("        cwd: %s\n", cwd);
  }
}

void print_cursor(Cursor *cursor) {
  printf("          mouse_active: %s\n",
         cursor->mouse_active ? "True" : "False");
  printf("          shape: ");
  print_cursor_shape(cursor->shape);
  printf("          visible: %s\n", cursor->visible ? "True" : "False");
}

void print_process(Process *process) {
  if (process->pid != -1) {
    printf("      Process: %s\n", process->name);
    printf("        pid: %d\n", process->pid);
    printf("        fd: %d\n", process->fd);
    printf("        closed: %s\n", process->closed ? "True" : "False");
    printf("        cursor:\n");
    print_cursor(&process->cursor);
    print_process_cwd(process->pid);
  } else {
    printf("      Process: %s\n", "None");
  }
}

void print_pane(Pane *pane) {
  printf("    Pane: %p\n", (void *)pane);
  printf("      col: %d\n", pane->col);
  printf("      row: %d\n", pane->row);
  printf("      width: %d\n", pane->width);
  printf("      height: %d\n", pane->height);
  print_process(pane->process);
}

void print_window(Window *window) {
  printf("  Window: %s\n", window->title);
  printf("    Pane Count: %d\n", window->pane_count);
  printf("    Current Pane: %d\n", window->current_pane);
  printf("    Layout: ");
  print_layout(window->layout);
  printf("\n");
  for (int j = 0; j < window->pane_count; j++) {
    Pane *pane = &window->panes[j];
    print_pane(pane);
  }
}

void print_session(Session *session) {
  printf("Session: %s\n", session->title);
  printf("  Window Count: %d\n", session->window_count);
  printf("  Current Window: %d\n", session->current_window);
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
