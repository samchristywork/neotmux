#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "lua.h"
#include "session.h"

extern Neotmux *neotmux;

enum BarPosition { BAR_NONE, BAR_TOP, BAR_BOTTOM };
int barPos = BAR_NONE;

VTermScreenCell prevCell = {0};

// TODO: Change the name of this struct
typedef struct BackBuffer {
  char *buffer;
  int n;
  int capacity;
} BackBuffer;
BackBuffer bb = {0};

#define bb_write(buf, count)                                                   \
  do {                                                                         \
    if (bb.n + count >= bb.capacity) {                                         \
      bb.capacity *= 2;                                                        \
      bb.buffer = realloc(bb.buffer, bb.capacity);                             \
    }                                                                          \
    memcpy(bb.buffer + bb.n, buf, count);                                      \
    bb.n += count;                                                             \
  } while (0)

unsigned int utf8_seqlen(long codepoint) {
  if (codepoint < 0x0000080) {
    return 1;
  } else if (codepoint < 0x0000800) {
    return 2;
  } else if (codepoint < 0x0010000) {
    return 3;
  } else if (codepoint < 0x0200000) {
    return 4;
  } else if (codepoint < 0x4000000) {
    return 5;
  } else {
    return 6;
  }
}

int fill_utf8(long codepoint, char *str) {
  int nbytes = utf8_seqlen(codepoint);

  int b = nbytes;
  while (b > 1) {
    b--;
    str[b] = 0x80 | (codepoint & 0x3f);
    codepoint >>= 6;
  }

  switch (nbytes) {
  case 1:
    str[0] = (codepoint & 0x7f);
    break;
  case 2:
    str[0] = 0xc0 | (codepoint & 0x1f);
    break;
  case 3:
    str[0] = 0xe0 | (codepoint & 0x0f);
    break;
  case 4:
    str[0] = 0xf0 | (codepoint & 0x07);
    break;
  case 5:
    str[0] = 0xf8 | (codepoint & 0x03);
    break;
  case 6:
    str[0] = 0xfc | (codepoint & 0x01);
    break;
  }

  return nbytes;
}

bool is_in_rect(int row, int col, int rowRect, int colRect, int height,
                int width) {
  return row >= rowRect && row < rowRect + height && col >= colRect &&
         col < colRect + width;
}

bool compare_colors(VTermColor a, VTermColor b) {
  if (a.type != b.type) {
    return false;
  } else if (a.type == VTERM_COLOR_DEFAULT_FG ||
             a.type == VTERM_COLOR_DEFAULT_BG) {
    return true;
  } else if (VTERM_COLOR_IS_INDEXED(&a) && VTERM_COLOR_IS_INDEXED(&b)) {
    return a.indexed.idx == b.indexed.idx;
  } else if (VTERM_COLOR_IS_RGB(&a) && VTERM_COLOR_IS_RGB(&b)) {
    return a.rgb.red == b.rgb.red && a.rgb.green == b.rgb.green &&
           a.rgb.blue == b.rgb.blue;
  } else {
    return false;
  }
}

void bb_write_rgb(int red, int green, int blue, int type);

// TODO: Interpolate into rgb colors
void bb_write_indexed(int idx, int type) {
  if (idx == 1) {
    bb_write_rgb(255, 0, 0, type);
    return;
  }

  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;5;%dm", type, idx);
  bb_write(buf, n);
}

void render_cell(VTermScreenCell cell) {
  if (cell.attrs.bold != prevCell.attrs.bold) {
    if (cell.attrs.bold) {
      bb_write("\033[1m", 4); // Enable bold
    } else {
      bb_write("\033[22m", 5); // Disable bold
    }
    prevCell.attrs.bold = cell.attrs.bold;
  }

  if (cell.attrs.reverse != prevCell.attrs.reverse) {
    if (cell.attrs.reverse) {
      bb_write("\033[7m", 4); // Enable reverse
    } else {
      bb_write("\033[27m", 5); // Disable reverse
    }
    prevCell.attrs.reverse = cell.attrs.reverse;
  }

  if (cell.attrs.underline != prevCell.attrs.underline) {
    if (cell.attrs.underline) {
      bb_write("\033[4m", 4); // Enable underline
    } else {
      bb_write("\033[24m", 5); // Disable underline
    }
    prevCell.attrs.underline = cell.attrs.underline;
  }

  if (cell.attrs.italic != prevCell.attrs.italic) {
    if (cell.attrs.italic) {
      bb_write("\033[3m", 4); // Enable italic
    } else {
      bb_write("\033[23m", 5); // Disable italic
    }
    prevCell.attrs.italic = cell.attrs.italic;
  }

  if (cell.attrs.strike != prevCell.attrs.strike) {
    if (cell.attrs.strike) {
      bb_write("\033[9m", 4); // Enable strike
    } else {
      bb_write("\033[29m", 5); // Disable strike
    }
    prevCell.attrs.strike = cell.attrs.strike;
  }

  if (!compare_colors(cell.bg, prevCell.bg)) {
    if (cell.bg.type == VTERM_COLOR_DEFAULT_BG) {
    } else if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
      bb_write_indexed(cell.bg.indexed.idx, 48);
      prevCell.bg = cell.bg;
    } else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
      bb_write_rgb(cell.bg.rgb.red, cell.bg.rgb.green, cell.bg.rgb.blue, 48);
      prevCell.bg = cell.bg;
    }
  }

  if (!compare_colors(cell.fg, prevCell.fg)) {
    if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
      bb_write_indexed(cell.fg.indexed.idx, 38);
    } else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
      bb_write_rgb(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue, 38);
    }
    prevCell.fg = cell.fg;
  }

  if (cell.chars[0] == 0) {
    bb_write(" ", 1);
  } else {
    for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i]; i++) {
      char bytes[6];
      int len = fill_utf8(cell.chars[i], bytes);
      bytes[len] = 0;
      bb_write(bytes, len);
    }
  }
}

void bb_pos(int row, int col) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);
  bb_write(buf, n);
}

void bb_color(int color) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[38;5;%dm", color);
  bb_write(buf, n);
}

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
void status_bar(int cols, int row) {
  bb_pos(row, 1);

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

void clear_style() {
  prevCell = (VTermScreenCell){0};
  bb_write("\033[0m", 4); // Reset style
}

bool is_border_here(int row, int col, Window *window) {
  for (int i = 0; i < window->pane_count; i++) {
    Pane *pane = &window->panes[i];
    if (is_in_rect(row, col, pane->row, pane->col, pane->height, pane->width)) {
      return false;
    }
  }

  return true;
}

bool borders_active_pane(int row, int col, Window *window) {
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
  bool r = is_border_here(row, col + 1, window);
  bool l = is_border_here(row, col - 1, window);
  bool d = is_border_here(row + 1, col, window);
  bool u = is_border_here(row - 1, col, window);

  if (u && d && l && r) {
    bb_write("┼", 3);
  } else if (u && d && l && !r) {
    bb_write("┤", 3);
  } else if (u && d && !l && r) {
    bb_write("├", 3);
  } else if (u && d && !l && !r) {
    bb_write("│", 3);
  } else if (u && !d && l && r) {
    bb_write("┴", 3);
  } else if (u && !d && l && !r) {
    bb_write("┘", 3);
  } else if (u && !d && !l && r) {
    bb_write("└", 3);
  } else if (!u && d && l && r) {
    bb_write("┬", 3);
  } else if (!u && d && l && !r) {
    bb_write("┐", 3);
  } else if (!u && d && !l && r) {
    bb_write("┌", 3);
  } else if (!u && !d && l && r) {
    bb_write("─", 3);
  } else {
    bb_write(" ", 1);
  }
}

void info_bar(int row) {
  bb_pos(row, 1);
  bb_write("\033[2K", 4); // Clear line
  char *result = function_to_string(neotmux->lua, "neotmux_info");
  if (result != NULL) {
    bb_write(result, strlen(result));
    free(result);
  }
}

void render_screen(int fd, int rows, int cols) {
  if (barPos == BAR_NONE) {
    barPos = BAR_BOTTOM;
    char *bp = get_global_string(neotmux->lua, "bar_position");
    if (bp != NULL) {
      if (strcmp(bp, "top") == 0) {
        barPos = BAR_TOP;
      } else if (strcmp(bp, "bottom") == 0) {
        barPos = BAR_BOTTOM;
      }
      free(bp);
    }
  }

  if (bb.buffer == NULL) {
    bb.buffer = malloc(100);
    bb.capacity = 100;
  }

  // This may fix weird artifacts in E.g. `man ls`
  // TODO: Potential performance improvement
  bzero(bb.buffer, bb.capacity);

  bb.n = 0;

  bb_write("\033[H", 3); // Move cursor to top left
  if (barPos == BAR_TOP) {
    bb_write("\r\n", 2);
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

        if (borders_active_pane(row, col, currentWindow)) {
          static int border_color = -1;
          if (border_color == -1) {
            border_color = get_global_int(neotmux->lua, "border_color");
            if (border_color == 0) {
              border_color = 4;
            }
          }
          bb_color(border_color);
          write_border_character(row, col, currentWindow);
          bb_write("\033[0m", 4);
        } else {
          write_border_character(row, col, currentWindow);
        }
      }
    }
    if (row < rows - 1) {
      clear_style();
      bb_write("\r\n", 2);
    }
  }

  // TODO: This should be done asynchronously
  if (barPos == BAR_TOP) {
    status_bar(cols, 1);
  } else if (barPos == BAR_BOTTOM) {
    status_bar(cols, rows + 1);
  }

  info_bar(rows + 2);

  {
    VTermPos cursorPos;
    VTermState *state = vterm_obtain_state(current_pane->process.vt);
    vterm_state_get_cursorpos(state, &cursorPos);
    cursorPos.row += currentPane->row;
    cursorPos.col += currentPane->col;

    if (barPos == BAR_TOP) {
      cursorPos.row++;
    }

    bb_write("\033[", 2);
    char buf[32];
    int n = snprintf(buf, 32, "%d;%dH", cursorPos.row + 1, cursorPos.col + 1);
    bb_write(buf, n);
  }

  if (current_pane->process.cursor_visible) {
    bb_write("\033[?25h", 6); // Show cursor
  } else {
    bb_write("\033[?25l", 6); // Hide cursor
  }

  switch (current_pane->process.cursor_shape) {
  case VTERM_PROP_CURSORSHAPE_BLOCK:
    bb_write("\033[0 q", 6); // Block cursor
    break;
  case VTERM_PROP_CURSORSHAPE_UNDERLINE:
    bb_write("\033[3 q", 6); // Underline cursor
    break;
  case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
    bb_write("\033[5 q", 6); // Vertical bar cursor
    break;
  default:
    break;
  }

  write(fd, bb.buffer, bb.n);
}
