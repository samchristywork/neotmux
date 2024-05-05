#include <stdio.h>
#include <unistd.h>

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
    lua_getglobal(neotmux->lua, "bar_color");
    if (lua_isnumber(neotmux->lua, -1)) {
      bar_color = lua_tointeger(neotmux->lua, -1);
    } else {
      bar_color = 0;
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

#define write_position(row, col)                                               \
  char buf[32];                                                                \
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);                          \
  buf_write(buf, n);

void write_cursor_position() {
  VTermPos cursorPos;
  Pane *currentPane = get_current_pane(neotmux);
  if (currentPane) {
    VTermState *state = vterm_obtain_state(currentPane->process->vt);
    vterm_state_get_cursorpos(state, &cursorPos);
    cursorPos.row += currentPane->row;
    cursorPos.col += currentPane->col;

    if (neotmux->barPos == BAR_TOP) {
      cursorPos.row++;
    }

    write_position(cursorPos.row + 1, cursorPos.col + 1);
  }
}

void write_cursor_style() {
  Pane *currentPane = get_current_pane(neotmux);
  if (currentPane) {
    if (currentPane->process->cursor.visible) {
      buf_write("\033[?25h", 6); // Show cursor
    } else {
      buf_write("\033[?25l", 6); // Hide cursor
    }

    switch (currentPane->process->cursor.shape) {
    case VTERM_PROP_CURSORSHAPE_BLOCK:
      buf_write("\033[0 q", 6); // Block cursor
      break;
    case VTERM_PROP_CURSORSHAPE_UNDERLINE:
      buf_write("\033[3 q", 6); // Underline cursor
      break;
    case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
      buf_write("\033[5 q", 6); // Vertical bar cursor
      break;
    default:
      break;
    }
  }
}

void render_bar(int fd) {
  neotmux->bb.n = 0;

  Session *currentSession = &neotmux->sessions[neotmux->current_session];
  Window *currentWindow =
      &currentSession->windows[currentSession->current_window];
  int cols = currentWindow->width;
  int rows = currentWindow->height;

  if (neotmux->barPos == BAR_TOP) {
    write_position(1, 1);
    write_status_bar(cols);
  } else if (neotmux->barPos == BAR_BOTTOM) {
    write_position(rows + 1, 1);
    write_status_bar(cols);
  }

  write_cursor_position();
  write_cursor_style();

  write(fd, neotmux->bb.buffer, neotmux->bb.n);
}
