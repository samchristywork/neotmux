#include <stdio.h>
#include <unistd.h>

#include "render_cell.h"
#include "session.h"

extern Neotmux *neotmux;

char *call_statusbar_function(int cols, lua_State *lua, char *sessionName,
                              Window *windows, int windowCount,
                              Window *currentWindow) {
  lua_getglobal(lua, "statusbar");
  if (!lua_isfunction(lua, -1)) {
    fprintf(neotmux->log, "statusbar is not a function\n");
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
void write_status_bar(int cols) {
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
      cols, neotmux->lua, session->title, session->windows,
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
      render_cell(cell);
    }
  }
}

#define write_position(row, col)                                               \
  char buf[32];                                                                \
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);                          \
  buf_write(buf, n);

bool write_cursor_position() {
  VTermPos cursorPos;
  Pane *currentPane = get_current_pane(neotmux);
  if (currentPane) {
    VTermState *state = vterm_obtain_state(currentPane->process->vt);
    vterm_state_get_cursorpos(state, &cursorPos);
    cursorPos.row += currentPane->row - currentPane->process->scrolloffset;
    cursorPos.col += currentPane->col;

    if (neotmux->barPos == BAR_TOP) {
      cursorPos.row++;
    }

    if (cursorPos.row < 0 ||
        cursorPos.row >= currentPane->height + currentPane->row) {
      buf_write("\033[?25l", 6); // Hide cursor
      return false;
    } else {
      write_position(cursorPos.row + 1, cursorPos.col + 1);
      return true;
    }
  }

  return false;
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
  buf_write("\033[?25l", 6); // Hide cursor

  Window *currentWindow = get_current_window(neotmux);
  int cols = currentWindow->width;
  int rows = currentWindow->height;

  if (neotmux->barPos == BAR_TOP) {
    write_position(1, 1);
    write_status_bar(cols);
  } else if (neotmux->barPos == BAR_BOTTOM) {
    write_position(rows + 1, 1);
    write_status_bar(cols);
  }

  bool show_cursor = write_cursor_position();
  if (show_cursor) {
    write_cursor_style();
  }

  write(fd, neotmux->bb.buffer, neotmux->bb.n);
}
