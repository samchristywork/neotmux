#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "session.h"

extern Neotmux *neotmux;

int physical_width(char *str) {
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
void status_bar(int cols) {
  int width = 0;
  Session *session = &neotmux->sessions[neotmux->current_session];
  Window *current_window = &session->windows[session->current_window];

  bb_write("\033[7m", 4); // Invert colors
  static int bar_color = -1;
  if (bar_color == -1) {
    bar_color = get_global_int(neotmux->lua, "bar_color");
    if (bar_color == 0) {
      bar_color = 4;
    }
  }
  bb_color(bar_color);

  char *sessionName = session->title;
  bb_write("[", 1);
  width++;

  bb_write(sessionName, strlen(sessionName));
  width += strlen(sessionName);

  bb_write("]", 1);
  width += 1;

  for (int i = 0; i < session->window_count; i++) {
    Window *window = &session->windows[i];
    bb_write("  ", 2);
    width += 2;

    bb_write(window->title, strlen(window->title));
    width += strlen(window->title);

    if (window == current_window) {
      bb_write("*", 1);
      width++;
    }

    if (window->zoom != -1) {
      bb_write("Z", 1);
      width += 1;
    }
  }

  char *statusRight = function_to_string(neotmux->lua, "status_right");
  if (statusRight == NULL) {
    statusRight = strdup("");
  }
  width += physical_width(statusRight);

  char padding[cols - width];
  memset(padding, ' ', cols - width);

  bb_write(padding, cols - width);
  bb_write(statusRight, strlen(statusRight));
  bb_write("\033[0m", 4);
  free(statusRight);
}
