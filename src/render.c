#include <render.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

// TODO: Determine the correct buffer size
#define BUF_SIZE 100000

enum BarPosition { TOP, BOTTOM };

int barPos = TOP;

typedef struct BackBuffer {
  char buffer[BUF_SIZE];
  int n;
} BackBuffer;

BackBuffer bb;

void bbWrite(const void *buf, size_t count) {
  memcpy(bb.buffer + bb.n, buf, count);
  bb.n += count;
}

void bbClear() {
  bb.n = 0;
}

bool isInRect(int row, int col, int rowRect, int colRect, int height,
              int width) {
  return row >= rowRect && row < rowRect + height && col >= colRect &&
         col < colRect + width;
}

void renderCell(VTermScreenCell cell) {
  if (cell.attrs.bold) {
    bbWrite("\033[1m", 4);
  }

  if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
    char buf[BUF_SIZE];
    int n = snprintf(buf, BUF_SIZE, "\033[48;5;%dm", cell.bg.indexed.idx);
    bbWrite(buf, n);
  } else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
    char buf[BUF_SIZE];
    int n = snprintf(buf, BUF_SIZE, "\033[48;2;%d;%d;%dm", cell.bg.rgb.red,
                     cell.bg.rgb.green, cell.bg.rgb.blue);
    bbWrite(buf, n);
  }

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

  if (cell.chars[0] == 0) {
    bbWrite(" ", 1);
  } else {
    for(int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i]; i++) {
      char bytes[6];
      int len = fill_utf8(cell.chars[i], bytes);
      bytes[len] = 0;
      bbWrite(bytes, len);
    }
  }

  bbWrite("\033[0m", 4); // Reset colors
}

void infoBar(int rows, int cols) {
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
  int windowIndex = 0;
  char windowName[] = "bash";

  int n = snprintf(buf, cols, "[%s] ", sessionName);
  bbWrite(buf, n);
  idx += n;

  n = snprintf(buf, cols, "%d:%s*", windowIndex, windowName);
  bbWrite(buf, n);
  idx += n;

  char *statusRight = "Hello, World!";
  idx += strlen(statusRight);

  for (int i = idx; i < cols; i++) {
    bbWrite(" ", 1);
  }

  bbWrite(statusRight, strlen(statusRight));

  bbWrite("\033[0m", 4);
  bbWrite("\r\n", 2);
}

void renderScreen(int fd, Window *windows, int nWindows, int activeTerm,
                  int rows, int cols) {
  bb.n = 0;

  bbWrite("\033[H", 3); // Move cursor to top left

  infoBar(rows, cols);

  if (barPos == TOP) {
    statusBar(cols);
  }

  VTermPos cursorPos;
  vterm_state_get_cursorpos(vterm_obtain_state(windows[activeTerm].vt),
                            &cursorPos);

  for (int row = 0; row < rows - 1; row++) {
    for (int col = 0; col < cols; col++) {
      if (cursorPos.row == row - windows[activeTerm].row &&
          cursorPos.col == col - windows[activeTerm].col) {
        bbWrite("\033[7m", 4); // Invert colors
      }
      bool isRendered = false;
      for (int k = 0; k < nWindows; k++) {
        if (windows[k].closed) {
          continue;
        }

        if (isInRect(row, col, windows[k].row, windows[k].col,
                     windows[k].height, windows[k].width)) {
          VTermPos pos;
          pos.row = row - windows[k].row;
          pos.col = col - windows[k].col;
          VTermScreen *vts = windows[k].vts;

          VTermScreenCell cell = {0};
          vterm_screen_get_cell(vts, pos, &cell);
          renderCell(cell);
          isRendered = true;
          break;
        }
      }
      if (!isRendered) {
        bbWrite("?", 1);
      }
      if (cursorPos.row == row - windows[activeTerm].row &&
          cursorPos.col == col - windows[activeTerm].col) {
        bbWrite("\033[0m", 4);
      }
    }
    bbWrite("\r\n", 2);
  }

  if (barPos == BOTTOM) {
    statusBar(cols);
  }

  write(fd, bb.buffer, bb.n);
}
