#include <stdio.h>
#include <stdlib.h>

#include "session.h"

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
