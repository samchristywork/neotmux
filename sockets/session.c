#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "session.h"

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

void print_process(Process *process) {
  if (process->pid != -1) {
    printf("      Process: %s\n", process->name);
    printf("        pid: %d\n", process->pid);
    printf("        fd: %d\n", process->fd);
    printf("        closed: %s\n", process->closed ? "True" : "False");
    printf("        cursor_visible: %s\n",
           process->cursor_visible ? "True" : "False");
    printf("        cursor_shape: ");
    print_cursor_shape(process->cursor_shape);
    print_process_cwd(process->pid);
  } else {
    printf("      Process: %s\n", "None");
  }
}

void print_pane(Pane *pane) {
  printf("    Pane: %p\n", pane);
  printf("      col: %d\n", pane->col);
  printf("      row: %d\n", pane->row);
  printf("      width: %d\n", pane->width);
  printf("      height: %d\n", pane->height);
  print_process(&pane->process);
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

Pane *add_pane(Window *window, int col, int row, int width, int height) {
  int n = sizeof(*window->panes) * (window->pane_count + 1);
  window->panes = realloc(window->panes, n);
  Pane *pane = &window->panes[window->pane_count];
  pane->col = col;
  pane->row = row;
  pane->width = width;
  pane->height = height;
  pane->process.pid = -1;
  pane->process.name = NULL;
  pane->process.fd = -1;
  pane->process.closed = false;
  pane->process.cursor_visible = true;
  pane->process.mouse = VTERM_PROP_MOUSE_NONE;
  window->pane_count++;
  return pane;
}

Window *add_window(Session *session, char *title) {
  int n = sizeof(*session->windows) * (session->window_count + 1);
  session->windows = realloc(session->windows, n);
  Window *window = &session->windows[session->window_count];
  window->title = malloc(strlen(title) + 1);
  strcpy(window->title, title);
  window->pane_count = 0;
  window->current_pane = 0;
  window->panes = NULL;
  window->layout = LAYOUT_MAIN_VERTICAL;
  window->width = 80;
  window->height = 24;
  window->zoom = -1;
  session->window_count++;
  return window;
}

Session *add_session(Neotmux *neotmux, char *title) {
  if (neotmux->sessions == NULL) {
    neotmux->sessions = malloc(sizeof(*neotmux->sessions));
  } else {
    int n = sizeof(*neotmux->sessions) * (neotmux->session_count + 1);
    neotmux->sessions = realloc(neotmux->sessions, n);
  }
  Session *session = &neotmux->sessions[neotmux->session_count];
  session->title = malloc(strlen(title) + 1);
  strcpy(session->title, title);
  session->window_count = 0;
  session->current_window = 0;
  session->windows = NULL;
  neotmux->session_count++;
  return session;
}
