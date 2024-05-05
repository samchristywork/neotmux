#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "layout.h"
#include "move.h"
#include "pane.h"
#include "print_session.h"
#include "render.h"
#include "session.h"
#include "statusbar.h"

extern Neotmux *neotmux;

Pane *add_pane(Window *window) {
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

void handle_command(int socket, char *buf, int read_size) {
  char cmd[read_size];
  memcpy(cmd, buf + 1, read_size - 1);
  cmd[read_size - 1] = '\0';

  if (strcmp(cmd, "Init") == 0) {
    Session *s = add_session(neotmux, "Main");
    Window *w = add_window(s, "Main");
    for (int i = 0; i < 3; i++) {
      add_pane(w);
    }
  }

  Session *session = &neotmux->sessions[neotmux->current_session];
  Window *w = &session->windows[session->current_window];

  if (strcmp(cmd, "Create") == 0) {
    Session *session = &neotmux->sessions[neotmux->current_session];
    Window *window;

    lua_getglobal(neotmux->lua, "default_title");
    if (!lua_isstring(neotmux->lua, -1)) {
      window = add_window(session, "New Window");
    } else {
      char *name = (char *)lua_tostring(neotmux->lua, -1);
      window = add_window(session, name);
      lua_pop(neotmux->lua, 1);
    }

    Window *w = get_current_window(neotmux);
    window->width = w->width;
    window->height = w->height;
    add_pane(window);
    session->current_window = session->window_count - 1;

    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);
  } else if (strcmp(cmd, "RenderScreen") == 0) {
    render_screen(socket);
  } else if (strcmp(cmd, "RenderBar") == 0) {
    render_bar(socket);
  } else if (strcmp(cmd, "Layout") == 0) {
    // TODO: Fix
    // if (session->current_window > 0 &&
    //    session->current_window < session->window_count) {

    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);
    //}
  } else if (strcmp(cmd, "VSplit") == 0) {
    Window *w = get_current_window(neotmux);
    add_pane(w);
    calculate_layout(w);
    w->current_pane = w->pane_count - 1;
  } else if (strcmp(cmd, "Split") == 0) {
    Window *w = get_current_window(neotmux);
    add_pane(w);
    calculate_layout(w);
    w->current_pane = w->pane_count - 1;
  } else if (strcmp(cmd, "CycleStatus") == 0) {
    neotmux->statusBarIdx++;
  } else if (strcmp(cmd, "List") == 0) {
    print_sessions(neotmux);
  } else if (strcmp(cmd, "Next") == 0) {
    Session *session = get_current_session(neotmux);
    session->current_window++;
    if (session->current_window >= session->window_count) {
      session->current_window = 0;
    }
    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);
  } else if (strcmp(cmd, "Prev") == 0) {
    Session *session = get_current_session(neotmux);
    session->current_window--;
    if (session->current_window < 0) {
      session->current_window = session->window_count - 1;
    }
    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);
  } else if (strcmp(cmd, "Left") == 0) {
    Window *w = get_current_window(neotmux);
    w->current_pane = move_active_pane(LEFT, w);
  } else if (strcmp(cmd, "Right") == 0) {
    Window *w = get_current_window(neotmux);
    w->current_pane = move_active_pane(RIGHT, w);
  } else if (strcmp(cmd, "Up") == 0) {
    Window *w = get_current_window(neotmux);
    w->current_pane = move_active_pane(UP, w);
  } else if (strcmp(cmd, "Down") == 0) {
    Window *w = get_current_window(neotmux);
    w->current_pane = move_active_pane(DOWN, w);
  } else if (strcmp(cmd, "Even_Horizontal") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_EVEN_HORIZONTAL;
    calculate_layout(w);
  } else if (strcmp(cmd, "Even_Vertical") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_EVEN_VERTICAL;
    calculate_layout(w);
  } else if (strcmp(cmd, "Main_Horizontal") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_MAIN_HORIZONTAL;
    calculate_layout(w);
  } else if (strcmp(cmd, "Main_Vertical") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_MAIN_VERTICAL;
    calculate_layout(w);
  } else if (strcmp(cmd, "Tiled") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_TILED;
    calculate_layout(w);
  } else if (strcmp(cmd, "Custom") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_CUSTOM;
    calculate_layout(w);
  } else if (strcmp(cmd, "Zoom") == 0) {
    Window *w = get_current_window(neotmux);
    if (w->zoom == -1) {
      w->zoom = w->current_pane;
    } else {
      w->zoom = -1;
    }
    calculate_layout(w);
  } else if (strcmp(cmd, "ScrollUp") == 0) {
    Pane *p = get_current_pane(neotmux);
    vterm_keyboard_unichar(p->process->vt, VTERM_KEY_UP, 0);
  } else if (strcmp(cmd, "ScrollDown") == 0) {
    Pane *p = get_current_pane(neotmux);
    vterm_keyboard_unichar(p->process->vt, VTERM_KEY_DOWN, 0);
  } else if (memcmp(cmd, "RenameWindow", 12) == 0) {
    Window *window = get_current_window(neotmux);
    free(window->title);
    char *title = strdup(cmd + 13);
    window->title = title;
  } else if (memcmp(cmd, "RenameSession", 13) == 0) {
    Session *session = get_current_session(neotmux);
    free(session->title);
    char *title = strdup(cmd + 14);
    session->title = title;
  } else if (strcmp(cmd, "Reload") == 0) {
    close(socket);
    exit(EXIT_SUCCESS);
  } else {
    printf("Unhandled command (%d): %s\n", socket, cmd);
  }
}
