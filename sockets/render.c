#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "session.h"

extern Session *sessions;
extern int num_sessions;

enum BarPosition { TOP, BOTTOM };

int barPos = BOTTOM;
int bold = 0;
VTermColor bg = {0};
VTermColor fg = {0};

// TODO: Determine the correct buffer size
// This is less inefficient than it seems because the memory is statically
// allocated.
#define BUF_SIZE 100000

typedef struct BackBuffer {
  char buffer[BUF_SIZE];
  int n;
} BackBuffer;
BackBuffer bb;

#define bb_write(buf, count)                                                   \
  do {                                                                         \
    memcpy(bb.buffer + bb.n, buf, count);                                      \
    bb.n += count;                                                             \
  } while (0)

unsigned int utf8_seqlen(long codepoint) {
  if (codepoint < 0x0000080)
    return 1;
  if (codepoint < 0x0000800)
    return 2;
  if (codepoint < 0x0010000)
    return 3;
  if (codepoint < 0x0200000)
    return 4;
  if (codepoint < 0x4000000)
    return 5;
  return 6;
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
  }

  if (a.type == VTERM_COLOR_DEFAULT_FG || a.type == VTERM_COLOR_DEFAULT_BG) {
    return true;
  }

  if (VTERM_COLOR_IS_INDEXED(&a) && VTERM_COLOR_IS_INDEXED(&b)) {
    return a.indexed.idx == b.indexed.idx;
  }

  if (VTERM_COLOR_IS_RGB(&a) && VTERM_COLOR_IS_RGB(&b)) {
    return a.rgb.red == b.rgb.red && a.rgb.green == b.rgb.green &&
           a.rgb.blue == b.rgb.blue;
  }

  return false;
}

void bb_write_indexed(int idx, int type) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;5;%dm", type, idx);
  bb_write(buf, n);
}

void bb_write_rgb(int red, int green, int blue, int type) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;2;%d;%d;%dm", type, red, green, blue);
  bb_write(buf, n);
}

void render_cell(VTermScreenCell cell) {
  if (cell.attrs.bold != bold) {
    bb_write("\033[1m", 4);
    bold = cell.attrs.bold;
  }

  if (!compare_colors(cell.bg, bg)) {
    if (cell.bg.type == VTERM_COLOR_DEFAULT_BG) {
    } else if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
      bb_write_indexed(cell.bg.indexed.idx, 48);
    } else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
      bb_write_rgb(cell.bg.rgb.red, cell.bg.rgb.green, cell.bg.rgb.blue, 48);
    }
    bg = cell.bg;
  }

  if (!compare_colors(cell.fg, fg)) {
    if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
      bb_write_indexed(cell.fg.indexed.idx, 38);
    } else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
      bb_write_rgb(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue, 38);
    }
    fg = cell.fg;
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

void bb_color(int color) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[38;5;%dm", color);
  bb_write(buf, n);
}

// TODO: Handle overflow condition
void status_bar(int cols) {
  int width = 0;
  Window *current_window = &sessions->windows[sessions->current_window];

  bb_write("\033[7m", 4); // Invert colors
  bb_color(2);

  char *sessionName = sessions->title;
  bb_write("[", 1);
  width++;

  bb_write(sessionName, strlen(sessionName));
  width += strlen(sessionName);

  bb_write("]", 1);
  width += 1;

  for (int i = 0; i < sessions->window_count; i++) {
    Window *window = &sessions->windows[i];
    bb_write("  ", 2);
    width += 2;

    bb_write(window->title, strlen(window->title));
    width += strlen(window->title);

    if (window == current_window) {
      bb_write("*", 1);
      width++;
    }
  }

  char hostname[50];
  gethostname(hostname, 50);

  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  int hour = tm->tm_hour;
  int minute = tm->tm_min;
  int day = tm->tm_mday;
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  const char *month = months[tm->tm_mon];
  int year = tm->tm_year + 1900;

  char statusRight[100];
  snprintf(statusRight, 100, "\"%s\" %d:%02d %d-%s-%d", hostname, hour, minute,
           day, month, year);
  width += strlen(statusRight);

  char padding[cols - width];
  memset(padding, ' ', cols - width);

  bb_write(padding, cols - width);
  bb_write(statusRight, strlen(statusRight));
  bb_write("\033[0m", 4);
}

void clear_style() {
  bzero(&bg, sizeof(bg));
  bzero(&fg, sizeof(fg));
  bold = 0;
  bb_write("\033[0m", 4);
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

  if (false) {
  } else if (u && d && l && r) {
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

void render_screen(int fd) {
  bb.n = 0;

  int cols = 80;
  int rows = 12;

  bb_write("\033[H", 3); // Move cursor to top left

  if (barPos == TOP) {
    status_bar(cols);
    bb_write("\r\n", 2);
  }

  VTermPos cursorPos;
  Window *currentWindow = &sessions->windows[sessions->current_window];
  Pane *currentPane = &currentWindow->panes[currentWindow->current_pane];
  vterm_state_get_cursorpos(vterm_obtain_state(currentPane->process.vt),
                            &cursorPos);

  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      if (cursorPos.row == row - currentPane->row &&
          cursorPos.col == col - currentPane->col) {
        bb_write("\033[7m", 4); // Invert colors
      }
      bool isRendered = false;
      for (int k = 0; k < currentWindow->pane_count; k++) {
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
        write_border_character(row, col, currentWindow);
      }
      if (cursorPos.row == row - currentPane->row &&
          cursorPos.col == col - currentPane->col) {
        bb_write("\033[0m", 4);
      }
    }
    if (row < rows - 1) {
      clear_style();
      bb_write("\r\n", 2);
    }
  }

  if (barPos == BOTTOM) {
    bb_write("\r\n", 2);
    status_bar(cols);
  }

  static int numRenders = 0;
  char buf[100];
  int n = snprintf(buf, 100, "\nRendered %d times", numRenders);
  bb_write(buf, n);
  numRenders++;

  Pane *current_pane = &currentWindow->panes[currentWindow->current_pane];

  {
    VTermPos cursorPos;
    VTermState *state = vterm_obtain_state(current_pane->process.vt);
    vterm_state_get_cursorpos(state, &cursorPos);
    cursorPos.row += currentPane->row;
    cursorPos.col += currentPane->col;

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

  write(fd, bb.buffer, bb.n);
}
