#include "pane.h"
#include "session.h"

Pane *add_pane(Window *window, char *cmd) {
  int n = sizeof(*window->panes) * (window->pane_count + 1);
  window->panes = realloc(window->panes, n);
  Pane *pane = &window->panes[window->pane_count];
  pane->col = 0;
  pane->row = 0;
  pane->width = 80;
  pane->height = 24;
  pane->process = malloc(sizeof(*pane->process));
  pane->process->pid = -1;
  pane->process->name = NULL;
  pane->process->fd = -1;
  pane->process->closed = false;
  pane->process->cursor.visible = true;
  pane->process->cursor.mouse_active = VTERM_PROP_MOUSE_NONE;
  pane->process->cursor.shape = VTERM_PROP_CURSORSHAPE_BLOCK;
  pane->selection.active = false;
  window->pane_count++;

  add_process_to_pane(pane, cmd);
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
  window->layout = strdup("layout_main_vertical");
  window->width = 80;
  window->height = 24;
  window->zoom = -1;
  window->rerender = false;
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
