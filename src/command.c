#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "layout.h"
#include "log.h"
#include "move.h"
#include "pane.h"
#include "plugin.h"
#include "print_session.h"
#include "render.h"
#include "session.h"

extern Neotmux *neotmux;

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
  window->layout = LAYOUT_MAIN_VERTICAL;
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

// TODO: Move to module
void swap_panes(Window *w, int a, int b) {
  Pane tmp = w->panes[a];
  w->panes[a] = w->panes[b];
  w->panes[b] = tmp;
}

void handle_command(int socket, char *buf, int read_size) {
  char cmd[read_size];
  memcpy(cmd, buf + 1, read_size - 1);
  cmd[read_size - 1] = '\0';

  WRITE_LOG(socket, "%s", cmd);

  if (strcmp(cmd, "Init") == 0) {
    load_plugins(socket);

    Session *s = add_session(neotmux, "Main");
    Window *w = add_window(s, "Main");

    int initial_panes = 3;
    lua_getglobal(neotmux->lua, "initial_panes");
    if (lua_isnumber(neotmux->lua, -1)) {
      initial_panes = lua_tonumber(neotmux->lua, -1);
    }
    lua_pop(neotmux->lua, 1);

    for (int i = 0; i < initial_panes; i++) {
      add_pane(w, NULL);
    }
    return;
  }

  if (strcmp(cmd, "Create") == 0) {
    Session *session = get_current_session(neotmux);
    if (session == NULL) {
      return;
    }

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
    add_pane(window, NULL);
    session->current_window = session->window_count - 1;

    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);
  } else if (strcmp(cmd, "RenderScreen") == 0) {
    // TODO: Implement double buffering
    // TODO: Why do we send 900+ bytes for a single char update?
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);
    render(socket, RENDER_SCREEN);
    gettimeofday(&end, NULL);

    static struct timeval programStart;
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    static int renderCount = 0;
    if (renderCount > 300) {
      renderCount = 0;
    }
    if (renderCount == 0) {
      gettimeofday(&programStart, NULL);
    }
    renderCount++;

    float elapsedTime = 0;
    elapsedTime += currentTime.tv_sec - programStart.tv_sec;
    elapsedTime += (currentTime.tv_usec - programStart.tv_usec) / 1000000.0;

    float renderTime = 0;
    renderTime = (end.tv_sec - start.tv_sec) * 1000.0;
    renderTime += (end.tv_usec - start.tv_usec) / 1000.0;

    WRITE_LOG(socket, "Render took %f ms", renderTime);
    WRITE_LOG(socket, "Renders: %d", renderCount);
    WRITE_LOG(socket, "Seconds: %f", elapsedTime);
    WRITE_LOG(socket, "Renders/Sec: %f", (float)renderCount / elapsedTime);
  } else if (strcmp(cmd, "RenderBar") == 0) {
    render(socket, RENDER_BAR);
  } else if (strcmp(cmd, "Layout") == 0) {
    // TODO: Fix
    // if (session->current_window > 0 &&
    //    session->current_window < session->window_count) {

    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);
    //}
  } else if (strcmp(cmd, "VSplit") == 0) {
    Window *w = get_current_window(neotmux);
    add_pane(w, NULL);
    calculate_layout(w);
    w->current_pane = w->pane_count - 1;
    w->zoom = -1;
  } else if (strcmp(cmd, "Split") == 0) {
    Window *w = get_current_window(neotmux);
    add_pane(w, NULL);
    calculate_layout(w);
    w->current_pane = w->pane_count - 1;
    w->zoom = -1;
  } else if (strcmp(cmd, "CycleStatus") == 0) {
    neotmux->statusBarIdx++;
  } else if (strcmp(cmd, "List") == 0) {
    print_sessions(neotmux, socket);
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
  } else if (strcmp(cmd, "SwapLeft") == 0) {
    Window *w = get_current_window(neotmux);
    int swap = move_active_pane(LEFT, w);
    swap_panes(w, w->current_pane, swap);
    w->current_pane = swap;
    calculate_layout(w);
  } else if (strcmp(cmd, "SwapRight") == 0) {
    Window *w = get_current_window(neotmux);
    int swap = move_active_pane(RIGHT, w);
    swap_panes(w, w->current_pane, swap);
    w->current_pane = swap;
    calculate_layout(w);
  } else if (strcmp(cmd, "SwapUp") == 0) {
    Window *w = get_current_window(neotmux);
    int swap = move_active_pane(UP, w);
    swap_panes(w, w->current_pane, swap);
    w->current_pane = swap;
    calculate_layout(w);
  } else if (strcmp(cmd, "SwapDown") == 0) {
    Window *w = get_current_window(neotmux);
    int swap = move_active_pane(DOWN, w);
    swap_panes(w, w->current_pane, swap);
    w->current_pane = swap;
    calculate_layout(w);
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
    // TODO: Fix the issue where mouse events are not being sent to the zoomed
    // window
    Window *w = get_current_window(neotmux);
    if (w->zoom == -1) {
      w->zoom = w->current_pane;
    } else {
      w->zoom = -1;
    }
    calculate_layout(w);
  } else if (strcmp(cmd, "Log") == 0) {
    printf("Log\n");
  } else if (strcmp(cmd, "ScrollUp") == 0) {
    Pane *p = get_current_pane(neotmux);
    p->process->scrolloffset += 1;
  } else if (strcmp(cmd, "ScrollDown") == 0) {
    Pane *p = get_current_pane(neotmux);
    p->process->scrolloffset -= 1;
  } else if (memcmp(cmd, "CreateNamed", 11) == 0) {
    Session *session = get_current_session(neotmux);
    if (session == NULL) {
      return;
    }

    char *title = strdup(cmd + 12);
    Window *window = add_window(session, title);

    Window *w = get_current_window(neotmux);
    window->width = w->width;
    window->height = w->height;
    add_pane(window, NULL);
    session->current_window = session->window_count - 1;

    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);
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
  } else if (strcmp(cmd, "ReloadLua") == 0) {
    load_plugins(socket);
  } else if (strcmp(cmd, "Quit") == 0) {
    close(socket);
    exit(0);
  } else {
    WRITE_LOG(socket, "Unhandled command: %s\n", cmd);
  }
}
