#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <vterm.h>
#include <client.h>

void makeCursorInvisible() { printf("\033[?25l"); }

void makeCursorVisible() { printf("\033[?25h"); }

void alternateScreen() { printf("\033[?1049h"); }

void normalScreen() { printf("\033[?1049l"); }

struct termios ttySetRaw() {
  struct termios t;
  if (tcgetattr(STDIN_FILENO, &t) == -1) {
    exit(EXIT_FAILURE);
  }

  struct termios oldTermios = t;

  t.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
  t.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK | ISTRIP |
                 IXON | PARMRK);
  t.c_oflag &= ~OPOST;
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) {
    exit(EXIT_FAILURE);
  }

  return oldTermios;
}

bool isInRect(int row, int col, int rowRect, int colRect, int height, int width) {
  return row >= rowRect && row < rowRect + height && col >= colRect && col < colRect + width;
}

void renderCell(VTermScreenCell cell) {
  bool needsReset = false;

  if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
    printf("\033[48;5;%dm", cell.bg.indexed.idx);
    needsReset = true;
  } else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
    printf("\033[48;2;%d;%d;%dm", cell.bg.rgb.red, cell.bg.rgb.green,
           cell.bg.rgb.blue);
    needsReset = true;
  }

  if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
    printf("\033[38;5;%dm", cell.fg.indexed.idx);
    needsReset = true;
  } else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
    printf("\033[38;2;%d;%d;%dm", cell.fg.rgb.red, cell.fg.rgb.green,
           cell.fg.rgb.blue);
    needsReset = true;
  }

  char c = cell.chars[0];
  if (cell.width == 1 && (isalnum(c) || c == ' ' || ispunct(c))) {
    printf("%c", c);
  } else if (c == 0) {
    printf(" ");
  } else {
    printf("?");
  }

  if (needsReset) {
    printf("\033[0m");
  }
}

void renderScreen(Window *windows, int nWindows, int activeTerm, int rows, int cols) {
  printf("\033[H"); // Move cursor to top left

  VTermPos cursorPos;
  vterm_state_get_cursorpos(vterm_obtain_state(windows[activeTerm].vt),
                            &cursorPos);

  for (int row = 0; row < rows - 1; row++) {
    // printf("\033[K"); // Clear line
    for (int col = 0; col < cols; col++) {
      if (cursorPos.row == row - windows[activeTerm].row &&
          cursorPos.col == col - windows[activeTerm].col) {
        printf("\033[7m");
      }
      bool isRendered = false;
      for (int k = 0; k < nWindows; k++) {
        if (isInRect(row, col,
              windows[k].row, windows[k].col, windows[k].height, windows[k].width
              )) {
          VTermPos pos;
          pos.row = row - windows[k].row;
          pos.col = col - windows[k].col;
          VTermScreen *vts = windows[k].vts;

          VTermScreenCell cell;
          vterm_screen_get_cell(vts, pos, &cell);
          renderCell(cell);
          isRendered = true;
          break;
        }
      }
      if (!isRendered) {
        printf("?");
      }
      if (cursorPos.row == row - windows[activeTerm].row &&
          cursorPos.col == col - windows[activeTerm].col) {
        printf("\033[0m");
      }
    }
    printf("\r\n");
  }
}
