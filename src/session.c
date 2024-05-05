#include "session.h"

Session *get_current_session(Neotmux *neotmux) {
  if (neotmux->current_session < 0 ||
      neotmux->current_session >= neotmux->session_count) {
    return NULL;
  }
  return &neotmux->sessions[neotmux->current_session];
}

Window *get_current_window(Neotmux *neotmux) {
  Session *session = get_current_session(neotmux);
  if (session == NULL) {
    return NULL;
  }
  if (session->current_window < 0 ||
      session->current_window >= session->window_count) {
    return NULL;
  }
  return &session->windows[session->current_window];
}

Pane *get_current_pane(Neotmux *neotmux) {
  Window *window = get_current_window(neotmux);
  if (window == NULL) {
    return NULL;
  }
  if (window->current_pane < 0 || window->current_pane >= window->pane_count) {
    return NULL;
  }
  return &window->panes[window->current_pane];
}
