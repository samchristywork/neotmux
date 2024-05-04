#include <stdio.h>

#include "lua.h"
#include "session.h"

extern Neotmux *neotmux;

int calculate_printed_width(char *str) {
  int width = 0;
  for (int i = 0; str[i]; i++) {
    if (str[i] == '\033') {
      while (str[i] && str[i] != 'm') {
        i++;
      }
    } else {
      width++;
    }
  }
  return width;
}

// TODO: Handle overflow condition
void write_status_bar(int cols) {
  int width = 0;
  Session *session = &neotmux->sessions[neotmux->current_session];
  Window *current_window = &session->windows[session->current_window];

  buf_write("\033[7m", 4); // Invert colors
  static int bar_color = -1;
  if (bar_color == -1) {
    bar_color = get_lua_int(neotmux->lua, "bar_color");
    if (bar_color == 0) {
      bar_color = 4;
    }
  }
  buf_color(bar_color);

  char *sessionName = session->title;
  buf_write("[", 1);
  width++;

  buf_write(sessionName, strlen(sessionName));
  width += strlen(sessionName);

  buf_write("]", 1);
  width += 1;

  for (int i = 0; i < session->window_count; i++) {
    Window *window = &session->windows[i];
    buf_write("  ", 2);
    width += 2;

    buf_write(window->title, strlen(window->title));
    width += strlen(window->title);

    if (window == current_window) {
      buf_write("*", 1);
      width++;
    }

    if (window->zoom != -1) {
      buf_write("Z", 1);
      width += 1;
    }
  }

  char *statusRight = NULL;
  lua_getglobal(neotmux->lua, "status_right");
  if (lua_isfunction(neotmux->lua, -1)) {
    int idx = neotmux->statusBarIdx;
    lua_pushinteger(neotmux->lua, idx);
    lua_call(neotmux->lua, 1, 1);
    statusRight = strdup(lua_tostring(neotmux->lua, -1));
    lua_pop(neotmux->lua, 1);
  } else {
    statusRight = strdup("");
  }
  width += calculate_printed_width(statusRight);

  char padding[cols - width];
  memset(padding, ' ', cols - width);

  buf_write(padding, cols - width);
  buf_write(statusRight, strlen(statusRight));
  buf_write("\033[0m", 4);
  free(statusRight);
}
