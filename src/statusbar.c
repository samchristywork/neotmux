#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>

#include "log.h"
#include "render_cell.h"
#include "session.h"

extern Neotmux *neotmux;

char *call_statusbar_function(int socket, int cols, lua_State *lua,
                              char *sessionName, Window *windows,
                              int windowCount, Window *currentWindow) {
  lua_getglobal(lua, "statusbar");
  if (!lua_isfunction(lua, -1)) {
    WRITE_LOG(socket, "statusbar is not a function");
    lua_pop(lua, 1);
    return NULL;
  }

  int idx = neotmux->statusBarIdx;
  lua_pushinteger(neotmux->lua, idx);

  lua_pushinteger(lua, cols);

  lua_pushstring(lua, sessionName);

  lua_newtable(lua);
  for (int i = 0; i < windowCount; i++) {
    lua_pushinteger(lua, i + 1);
    lua_newtable(lua);

    lua_pushstring(lua, "title");
    lua_pushstring(lua, windows[i].title);
    lua_settable(lua, -3);

    lua_pushstring(lua, "active");
    lua_pushboolean(lua, &windows[i] == currentWindow);
    lua_settable(lua, -3);

    lua_pushstring(lua, "zoom");
    lua_pushboolean(lua, windows[i].zoom >= 0);
    lua_settable(lua, -3);

    lua_settable(lua, -3);
  }

  lua_call(lua, 4, 1);
  char *statusbar = strdup(lua_tostring(lua, -1));
  lua_pop(lua, 1);
  return statusbar;
}

VTerm *vt = NULL;
VTermScreen *vts;

// TODO: Ensure line gets cleared
void write_status_bar(int socket, int cols) {
  if (vt == NULL) {
    vt = vterm_new(1, 1);
    vts = vterm_obtain_screen(vt);
    vterm_set_size(vt, 1, 1000); // Essentially disables wrap

    static VTermScreenCallbacks callbacks;
    callbacks.damage = NULL;
    callbacks.moverect = NULL;
    callbacks.movecursor = NULL;
    callbacks.settermprop = NULL;
    callbacks.bell = NULL;
    callbacks.resize = NULL;
    callbacks.sb_pushline = NULL;
    callbacks.sb_popline = NULL;

    vterm_set_utf8(vt, 1);
    vterm_screen_reset(vts, 1);
    vterm_screen_set_callbacks(vts, &callbacks, NULL);
  }

  Session *session = get_current_session(neotmux);
  Window *current_window = get_current_window(neotmux);

  char *statusbar = call_statusbar_function(
      socket, cols, neotmux->lua, session->title, session->windows,
      session->window_count, current_window);

  if (statusbar) {
    vterm_input_write(vt, "\033[0m", 4);      // Reset colors
    vterm_input_write(vt, "\033[7m", 4);      // Enable reverse
    vterm_input_write(vt, "\033[38;5;4m", 9); // Set foreground color
    vterm_input_write(vt, "\033[1;1H", 6);    // Move cursor to top left
    vterm_input_write(vt, "\033[2K", 4);      // Clear line
    vterm_input_write(vt, statusbar, strlen(statusbar));
    free(statusbar);

    VTermScreenCell cell;
    for (int i = 0; i < cols; i++) {
      VTermPos pos = {0};
      pos.col = i;
      pos.row = 0;
      vterm_screen_get_cell(vts, pos, &cell);

      if (neotmux->barPos == BAR_TOP) {
        render_cell(&cell, 1, i + 1);
      } else if (neotmux->barPos == BAR_BOTTOM) {
        render_cell(&cell, current_window->height + 1, i + 1);
      }
    }
  }
}

void render_bar(int fd) {
  buf_write("\033[?25l", 6); // Hide cursor

  Window *currentWindow = get_current_window(neotmux);
  int cols = currentWindow->width;

  write_status_bar(fd, cols);
}
