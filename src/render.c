#include <ctype.h>
#include <render.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// TODO: Determine the correct buffer size
#define BUF_SIZE 1024

enum BarPosition { TOP, BOTTOM };

int barPos = TOP;

bool isInRect(int row, int col, int rowRect, int colRect, int height,
              int width) {
  return row >= rowRect && row < rowRect + height && col >= colRect &&
         col < colRect + width;
}

void renderCell(int fd, VTermScreenCell cell) {
  if (cell.attrs.bold) {
    write(fd, "\033[1m", 4);
  }

  if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
    char buf[BUF_SIZE];
    int n = snprintf(buf, BUF_SIZE, "\033[48;5;%dm", cell.bg.indexed.idx);
    write(fd, buf, n);
  } else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
    char buf[BUF_SIZE];
    int n = snprintf(buf, BUF_SIZE, "\033[48;2;%d;%d;%dm", cell.bg.rgb.red,
                     cell.bg.rgb.green, cell.bg.rgb.blue);
    write(fd, buf, n);
  }

  if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
    char buf[BUF_SIZE];
    int n = snprintf(buf, BUF_SIZE, "\033[38;5;%dm", cell.fg.indexed.idx);
    write(fd, buf, n);
  } else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
    char buf[BUF_SIZE];
    int n = snprintf(buf, BUF_SIZE, "\033[38;2;%d;%d;%dm", cell.fg.rgb.red,
                     cell.fg.rgb.green, cell.fg.rgb.blue);
    write(fd, buf, n);
  }

  char c = cell.chars[0];
  c='0'+cell.chars[1];
  if (c == 0) {
    write(fd, " ", 1);
  } else {
    write(fd, &c, 1);
  }

  write(fd, "\033[0m", 4); // Reset colors
}

void infoBar(int fd, int rows, int cols) {
  write(fd, "\033[7m", 4); // Invert colors
  char buf[BUF_SIZE];
  static int frame = 0;
  int n = snprintf(buf, BUF_SIZE, "Rows: %d, Cols: %d, Frame: %d", rows, cols,
                   frame);
  frame++;
  write(fd, buf, n);
  write(fd, "\033[0m", 4);
  write(fd, "\r\n", 2);
}

void color(int fd, int color) {
  char buf[BUF_SIZE];
  int n = snprintf(buf, BUF_SIZE, "\033[38;5;%dm", color);
  write(fd, buf, n);
}

void statusBar(int fd, int cols) {
  write(fd, "\033[7m", 4); // Invert colors
  color(fd, 2);

  int idx = 0;
  char buf[cols];

  char sessionName[] = "0";
  int windowIndex = 0;
  char windowName[] = "bash";

  int n = snprintf(buf, cols, "[%s] ", sessionName);
  write(fd, buf, n);
  idx += n;

  n = snprintf(buf, cols, "%d:%s*", windowIndex, windowName);
  write(fd, buf, n);
  idx += n;

  char *statusRight = "Hello, World!";
  idx += strlen(statusRight);

  for (int i = idx; i < cols; i++) {
    write(fd, " ", 1);
  }

  write(fd, statusRight, strlen(statusRight));

  write(fd, "\033[0m", 4);
  write(fd, "\r\n", 2);
}

void renderScreen(int fd, Window *windows, int nWindows, int activeTerm,
                  int rows, int cols) {
  write(fd, "\033[H", 3); // Move cursor to top left

  infoBar(fd, rows, cols);

  if (barPos == TOP) {
    statusBar(fd, cols);
  }

  VTermPos cursorPos;
  vterm_state_get_cursorpos(vterm_obtain_state(windows[activeTerm].vt),
                            &cursorPos);

  for (int row = 0; row < rows - 1; row++) {
    // printf("\033[K"); // Clear line
    for (int col = 0; col < cols; col++) {
      if (cursorPos.row == row - windows[activeTerm].row &&
          cursorPos.col == col - windows[activeTerm].col) {
        write(fd, "\033[7m", 4); // Invert colors
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
          renderCell(fd, cell);
          isRendered = true;
          break;
        }
      }
      if (!isRendered) {
        write(fd, "?", 1);
      }
      if (cursorPos.row == row - windows[activeTerm].row &&
          cursorPos.col == col - windows[activeTerm].col) {
        write(fd, "\033[0m", 4);
      }
    }
    write(fd, "\r\n", 2);
  }

  if (barPos == BOTTOM) {
    statusBar(fd, cols);
  }
}
