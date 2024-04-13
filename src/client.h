#ifndef RENDER_H
#define RENDER_H

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <vterm.h>

typedef struct Rect {
  int width;
  int height;
  int col;
  int row;
} Rect;

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

bool isInRect(int row, int col, Rect *rect) {
  if (row < rect->row || row >= rect->row + rect->height) {
    return false;
  }

  if (col < rect->col || col >= rect->col + rect->width) {
    return false;
  }

  return true;
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

#endif
