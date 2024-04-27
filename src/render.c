#include <render.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <vterm.h>

// TODO: Determine the correct buffer size
#define BUF_SIZE 100000

enum BarPosition { TOP, BOTTOM };

int barPos = BOTTOM;

typedef struct BackBuffer {
  char buffer[BUF_SIZE];
  int n;
} BackBuffer;

BackBuffer bb;

void bbWrite(const void *buf, size_t count) {
  memcpy(bb.buffer + bb.n, buf, count);
  bb.n += count;
}

void bbClear() { bb.n = 0; }

bool isInRect(int row, int col, int rowRect, int colRect, int height,
              int width) {
  return row >= rowRect && row < rowRect + height && col >= colRect &&
         col < colRect + width;
}

bool compareColors(VTermColor a, VTermColor b) {
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

int bold = 0;
VTermColor bg = {0};
VTermColor fg = {0};

void renderCell(VTermScreenCell cell) {
  if (cell.attrs.bold != bold) {
    bbWrite("\033[1m", 4);
    bold = cell.attrs.bold;
  }

  if (!compareColors(cell.bg, bg)) {
    if (cell.bg.type == VTERM_COLOR_DEFAULT_BG) {
    } else if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
      char buf[BUF_SIZE];
      int n = snprintf(buf, BUF_SIZE, "\033[48;5;%dm", cell.bg.indexed.idx);
      bbWrite(buf, n);
    } else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
      char buf[BUF_SIZE];
      int n = snprintf(buf, BUF_SIZE, "\033[48;2;%d;%d;%dm", cell.bg.rgb.red,
                       cell.bg.rgb.green, cell.bg.rgb.blue);
      bbWrite(buf, n);
    }
    bg = cell.bg;
  }

  if (!compareColors(cell.fg, fg)) {
    if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
      char buf[BUF_SIZE];
      int n = snprintf(buf, BUF_SIZE, "\033[38;5;%dm", cell.fg.indexed.idx);
      bbWrite(buf, n);
    } else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
      char buf[BUF_SIZE];
      int n = snprintf(buf, BUF_SIZE, "\033[38;2;%d;%d;%dm", cell.fg.rgb.red,
                       cell.fg.rgb.green, cell.fg.rgb.blue);
      bbWrite(buf, n);
    }
    fg = cell.fg;
  }

  if (cell.chars[0] == 0) {
    bbWrite(" ", 1);
  } else {
    for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i]; i++) {
      char bytes[6];
      int len = fill_utf8(cell.chars[i], bytes);
      bytes[len] = 0;
      bbWrite(bytes, len);
    }
  }
}

void infoBar(int rows, int cols) {
  bbWrite("\033[0m", 4); // Reset colors
  bbWrite("\033[7m", 4); // Invert colors
  char buf[BUF_SIZE];
  static int frame = 0;
  int n = snprintf(buf, BUF_SIZE, "Rows: %d, Cols: %d, Frame: %d", rows, cols,
                   frame);
  frame++;
  bbWrite(buf, n);
  bbWrite("\033[0m", 4);
  bbWrite("\r\n", 2);
}

void color(int color) {
  char buf[BUF_SIZE];
  int n = snprintf(buf, BUF_SIZE, "\033[38;5;%dm", color);
  bbWrite(buf, n);
}

void statusBar(int cols) {
  bbWrite("\033[7m", 4); // Invert colors
  color(2);

  int idx = 0;
  char buf[cols];

  char sessionName[] = "0";
  int paneIndex = 0;
  char paneName[] = "bash";

  int n = snprintf(buf, cols, "[%s] ", sessionName);
  bbWrite(buf, n);
  idx += n;

  n = snprintf(buf, cols, "%d:%s*", paneIndex, paneName);
  bbWrite(buf, n);
  idx += n;

  char *statusRight = "Hello, World!";
  idx += strlen(statusRight);

  for (int i = idx; i < cols; i++) {
    bbWrite(" ", 1);
  }

  bbWrite(statusRight, strlen(statusRight));

  bbWrite("\033[0m", 4);
}

bool isInWindow(int row, int col, Pane *pane, int nPanes) {
  for (int i = 0; i < nPanes; i++) {
    if (isInRect(row, col, pane[i].row, pane[i].col, pane[i].height,
                 pane[i].width)) {
      return true;
    }
  }

  return false;
}

void writeBorderCharacter(int row, int col, Pane *panes, int nPanes) {

  bool r = isInWindow(row + 1, col, panes, nPanes) ||
           isInWindow(row - 1, col, panes, nPanes) ||
           (isInWindow(row + 1, col + 1, panes, nPanes) &&
            !isInWindow(row, col + 1, panes, nPanes)) ||
           (isInWindow(row - 1, col + 1, panes, nPanes) &&
            !isInWindow(row, col + 1, panes, nPanes));

  bool l = isInWindow(row + 1, col, panes, nPanes) ||
           isInWindow(row - 1, col, panes, nPanes) ||
           (isInWindow(row + 1, col - 1, panes, nPanes) &&
            !isInWindow(row, col - 1, panes, nPanes)) ||
           (isInWindow(row - 1, col - 1, panes, nPanes) &&
            !isInWindow(row, col - 1, panes, nPanes));

  bool d = isInWindow(row, col + 1, panes, nPanes) ||
           isInWindow(row, col - 1, panes, nPanes) ||
           (isInWindow(row + 1, col + 1, panes, nPanes) &&
            !isInWindow(row + 1, col, panes, nPanes)) ||
           (isInWindow(row + 1, col - 1, panes, nPanes) &&
            !isInWindow(row + 1, col, panes, nPanes));

  bool u = isInWindow(row, col + 1, panes, nPanes) ||
           isInWindow(row, col - 1, panes, nPanes) ||
           (isInWindow(row - 1, col + 1, panes, nPanes) &&
            !isInWindow(row - 1, col, panes, nPanes)) ||
           (isInWindow(row - 1, col - 1, panes, nPanes) &&
            !isInWindow(row - 1, col, panes, nPanes));

  if (false) {
  } else if (u && d && l && r) {
    bbWrite("┼", 3);
  } else if (u && d && l && !r) {
    bbWrite("┤", 3);
  } else if (u && d && !l && r) {
    bbWrite("├", 3);
  } else if (u && d && !l && !r) {
    bbWrite("│", 3);
  } else if (u && !d && l && r) {
    bbWrite("┴", 3);
  } else if (u && !d && l && !r) {
    bbWrite("┘", 3);
  } else if (u && !d && !l && r) {
    bbWrite("└", 3);
  } else if (!u && d && l && r) {
    bbWrite("┬", 3);
  } else if (!u && d && l && !r) {
    bbWrite("┐", 3);
  } else if (!u && d && !l && r) {
    bbWrite("┌", 3);
  } else if (!u && !d && l && r) {
    bbWrite("─", 3);
  } else {
    bbWrite(" ", 1);
  }
}

void renderScreen(int fd, Pane *panes, int nPanes, int activeTerm, int rows,
                  int cols) {
  bb.n = 0;

  bbWrite("\033[H", 3); // Move cursor to top left

  // infoBar(rows, cols);

  if (barPos == TOP) {
    statusBar(cols);
  }

  VTermPos cursorPos;
  vterm_state_get_cursorpos(vterm_obtain_state(panes[activeTerm].vt),
                            &cursorPos);

  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      if (cursorPos.row == row - panes[activeTerm].row &&
          cursorPos.col == col - panes[activeTerm].col) {
        bbWrite("\033[7m", 4); // Invert colors
      }
      bool isRendered = false;
      for (int k = 0; k < nPanes; k++) {
        if (panes[k].closed) {
          continue;
        }

        if (isInRect(row, col, panes[k].row, panes[k].col, panes[k].height,
                     panes[k].width)) {
          VTermPos pos;
          pos.row = row - panes[k].row;
          pos.col = col - panes[k].col;
          VTermScreen *vts = panes[k].vts;

          VTermScreenCell cell = {0};
          vterm_screen_get_cell(vts, pos, &cell);
          renderCell(cell);
          isRendered = true;
          break;
        }
      }
      if (!isRendered) {
        bzero(&bg, sizeof(bg));
        bzero(&fg, sizeof(fg));
        bold = 0;
        bbWrite("\033[0m", 4);
        char *s = "│";
        bbWrite(s, strlen(s));
      }
      if (cursorPos.row == row - panes[activeTerm].row &&
          cursorPos.col == col - panes[activeTerm].col) {
        bbWrite("\033[0m", 4);
      }
    }
    if (row < rows - 1) {
      bbWrite("\r\n", 2);
    }
  }

  if (barPos == BOTTOM) {
    statusBar(cols);
  }

  write(fd, bb.buffer, bb.n);
}
