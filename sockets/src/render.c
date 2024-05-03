#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "lua.h"
#include "render_cell.h"
#include "session.h"
#include "statusbar.h"

extern Neotmux *neotmux;

enum BarPosition { BAR_NONE, BAR_TOP, BAR_BOTTOM };
int barPos = BAR_NONE;

bool is_in_rect(int row, int col, int rowRect, int colRect, int height,
                int width) {
  return row >= rowRect && row < rowRect + height && col >= colRect &&
         col < colRect + width;
}

void write_position(int row, int col) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);
  buf_write(buf, n);
}

void clear_style() {
  bzero(&neotmux->prevCell, sizeof(neotmux->prevCell));
  buf_write("\033[0m", 4); // Reset style
}

bool is_border(int row, int col, Window *window) {
  for (int i = 0; i < window->pane_count; i++) {
    Pane *pane = &window->panes[i];
    if (is_in_rect(row, col, pane->row, pane->col, pane->height, pane->width)) {
      return false;
    }
  }

  return true;
}

bool is_bordering_active_pane(int row, int col, Window *window) {
  Pane *pane = &window->panes[window->current_pane];

  for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
      if (is_in_rect(row, col, pane->row + y, pane->col + x, pane->height,
                     pane->width)) {
        return true;
      }
    }
  }

  return false;
}

void write_border_character(int row, int col, Window *window) {
  bool r = is_border(row, col + 1, window);
  bool l = is_border(row, col - 1, window);
  bool d = is_border(row + 1, col, window);
  bool u = is_border(row - 1, col, window);

  if (u && d && l && r) {
    buf_write("┼", 3);
  } else if (u && d && l && !r) {
    buf_write("┤", 3);
  } else if (u && d && !l && r) {
    buf_write("├", 3);
  } else if (u && d && !l && !r) {
    buf_write("│", 3);
  } else if (u && !d && l && r) {
    buf_write("┴", 3);
  } else if (u && !d && l && !r) {
    buf_write("┘", 3);
  } else if (u && !d && !l && r) {
    buf_write("└", 3);
  } else if (!u && d && l && r) {
    buf_write("┬", 3);
  } else if (!u && d && l && !r) {
    buf_write("┐", 3);
  } else if (!u && d && !l && r) {
    buf_write("┌", 3);
  } else if (!u && !d && l && r) {
    buf_write("─", 3);
  } else {
    buf_write(" ", 1);
  }
}

void write_info_bar(int row) {
  write_position(row, 1);
  buf_write("\033[2K", 4); // Clear line
  char *result = apply_lua_string_function(neotmux->lua, "neotmux_info");
  if (result != NULL) {
    buf_write(result, strlen(result));
    free(result);
  }
}

void render_screen(int fd, int rows, int cols) {
  if (barPos == BAR_NONE) {
    barPos = BAR_BOTTOM;

    lua_getglobal(neotmux->lua, "bar_position");
    if (!lua_isstring(neotmux->lua, -1)) {
      lua_pop(neotmux->lua, 1);
    } else {
      if (strcmp(lua_tostring(neotmux->lua, -1), "top") == 0) {
        barPos = BAR_TOP;
      }
      lua_pop(neotmux->lua, 1);
    }
  }

  if (neotmux->bb.buffer == NULL) {
    neotmux->bb.buffer = malloc(100);
    neotmux->bb.capacity = 100;
  }

  // This may fix weird artifacts in E.g. `man ls`
  // TODO: Potential performance improvement
  bzero(neotmux->bb.buffer, neotmux->bb.capacity);

  neotmux->bb.n = 0;

  buf_write("\033[H", 3); // Move cursor to top left
  if (barPos == BAR_TOP) {
    buf_write("\r\n", 2);
  }

  for (int row = 0; row < rows; row++) {
    clear_style();
    for (int col = 0; col < cols; col++) {
      bool isRendered = false;
      Session *currentSession = &neotmux->sessions[neotmux->current_session];
      Window *currentWindow =
          &currentSession->windows[currentSession->current_window];
      for (int k = 0; k < currentWindow->pane_count; k++) {
        if (currentWindow->zoom != -1 && k != currentWindow->zoom) {
          continue;
        }

        Pane *pane = &currentWindow->panes[k];
        if (pane->process.closed) {
          continue;
        }

        if (is_in_rect(row, col, pane->row, pane->col, pane->height,
                       pane->width)) {
          VTermPos pos;
          pos.row = row - pane->row;
          pos.col = col - pane->col;
          VTermScreen *vts = pane->process.vts;

          VTermScreenCell cell = {0};
          vterm_screen_get_cell(vts, pos, &cell);
          render_cell(cell);
          isRendered = true;
          break;
        }
      }
      if (!isRendered) {
        clear_style();

        if (is_bordering_active_pane(row, col, currentWindow)) {
          static int border_color = -1;
          if (border_color == -1) {
            border_color = get_lua_int(neotmux->lua, "border_color");
            if (border_color == 0) {
              border_color = 4;
            }
          }
          buf_color(border_color);
          write_border_character(row, col, currentWindow);
          buf_write("\033[0m", 4);
        } else {
          write_border_character(row, col, currentWindow);
        }
      }
    }
    if (row < rows - 1) {
      clear_style();
      buf_write("\r\n", 2);
    }
  }

  // TODO: This should be done asynchronously
  if (barPos == BAR_TOP) {
    write_position(1, 1);
    write_status_bar(cols);
  } else if (barPos == BAR_BOTTOM) {
    write_position(rows + 1, 1);
    write_status_bar(cols);
  }

  write_info_bar(rows + 2);

  {
    VTermPos cursorPos;
    Session *currentSession = &neotmux->sessions[neotmux->current_session];
    Window *currentWindow =
        &currentSession->windows[currentSession->current_window];
    if (currentWindow->current_pane != -1) {
      Pane *currentPane = &currentWindow->panes[currentWindow->current_pane];
      VTermState *state = vterm_obtain_state(currentPane->process.vt);
      vterm_state_get_cursorpos(state, &cursorPos);
      cursorPos.row += currentPane->row;
      cursorPos.col += currentPane->col;

      if (barPos == BAR_TOP) {
        cursorPos.row++;
      }

      buf_write("\033[", 2);
      char buf[32];
      int n = snprintf(buf, 32, "%d;%dH", cursorPos.row + 1, cursorPos.col + 1);
      buf_write(buf, n);
    }
  }

  Session *currentSession = &neotmux->sessions[neotmux->current_session];
  Window *currentWindow =
      &currentSession->windows[currentSession->current_window];
  if (currentWindow->current_pane != -1) {
    Pane *currentPane = &currentWindow->panes[currentWindow->current_pane];
    if (currentPane->process.cursor_visible) {
      buf_write("\033[?25h", 6); // Show cursor
    } else {
      buf_write("\033[?25l", 6); // Hide cursor
    }

    switch (currentPane->process.cursor_shape) {
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

  write(fd, neotmux->bb.buffer, neotmux->bb.n);
}
