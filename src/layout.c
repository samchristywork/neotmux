#include <sys/ioctl.h>

#include "layout.h"

extern Neotmux *neotmux;

bool call_lua_layout_function(lua_State *L, const char *function, int *col,
                              int *row, int *width, int *height, int index,
                              int nPanes, int winWidth, int winHeight) {
  lua_getglobal(L, function);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 1);
    return false;
  }

  lua_getglobal(L, function);
  lua_pushinteger(L, index);
  lua_pushinteger(L, nPanes);
  lua_pushinteger(L, winWidth);
  lua_pushinteger(L, winHeight);
  lua_call(L, 4, 4);

  *col = lua_tointeger(L, -4);
  *row = lua_tointeger(L, -3);
  *width = lua_tointeger(L, -2);
  *height = lua_tointeger(L, -1);

  lua_pop(L, 4);
  return true;
}

#define update_layout(w)                                                       \
  for (int i = 0; i < w->pane_count; i++) {                                    \
    vterm_set_size(w->panes[i].process->vt, w->panes[i].height,                \
                   w->panes[i].width);                                         \
                                                                               \
    struct winsize ws;                                                         \
    ws.ws_row = w->panes[i].height;                                            \
    ws.ws_col = w->panes[i].width;                                             \
    ws.ws_xpixel = 0;                                                          \
    ws.ws_ypixel = 0;                                                          \
                                                                               \
    if (ioctl(w->panes[i].process->fd, TIOCSWINSZ, &ws) < 0) {                 \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  }

void apply_custom_layout(Window *w, char *name) {
  Pane *panes = w->panes;

  for (int i = 0; i < w->pane_count; i++) {
    int col, row, width, height;
    if (call_lua_layout_function(neotmux->lua, name, &col, &row, &width,
                                 &height, i, w->pane_count, w->width,
                                 w->height)) {
      panes[i].row = row;
      panes[i].col = col;
      panes[i].height = height;
      panes[i].width = width;
    }
  }

  update_layout(w);
}

void calculate_layout(Window *window) {
  if (!window) {
    return;
  }

  if (window->zoom != -1) {
    window->panes[window->zoom].row = 0;
    window->panes[window->zoom].col = 0;
    window->panes[window->zoom].height = window->height;
    window->panes[window->zoom].width = window->width;
    update_layout(window);
  } else {
    switch (window->layout) {
    case LAYOUT_DEFAULT:
      // TODO: Implement default layout
      break;
    case LAYOUT_EVEN_HORIZONTAL:
      apply_custom_layout(window, "layout_even_horizontal");
      break;
    case LAYOUT_EVEN_VERTICAL:
      apply_custom_layout(window, "layout_even_vertical");
      break;
    case LAYOUT_MAIN_HORIZONTAL:
      apply_custom_layout(window, "layout_main_horizontal");
      break;
    case LAYOUT_MAIN_VERTICAL:
      apply_custom_layout(window, "layout_main_vertical");
      break;
    case LAYOUT_TILED:
      //apply_tiled_layout(window);
      break;
    case LAYOUT_CUSTOM:
      // TODO: Fix how borders work to make this look correct
      apply_custom_layout(window, "layout_custom");
      break;
    default:
      break;
    }
  }
}
