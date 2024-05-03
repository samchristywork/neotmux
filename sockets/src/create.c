#include <string.h>

#include "pane.h"
#include "session.h"

Pane *add_pane(Window *window) {
  int n = sizeof(*window->panes) * (window->pane_count + 1);
  window->panes = realloc(window->panes, n);
  Pane *pane = &window->panes[window->pane_count];
  pane->col = 0;
  pane->row = 0;
  pane->width = 80;
  pane->height = 24;
  pane->process.pid = -1;
  pane->process.name = NULL;
  pane->process.fd = -1;
  pane->process.closed = false;
  pane->process.cursor_visible = true;
  pane->process.mouse = VTERM_PROP_MOUSE_NONE;
  window->pane_count++;

  add_process_to_pane(pane);
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
