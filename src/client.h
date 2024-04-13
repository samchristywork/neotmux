#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include <vterm.h>

typedef struct Rect {
  int width;
  int height;
  int col;
  int row;
} Rect;

void makeCursorInvisible();

void makeCursorVisible();

void alternateScreen();

void normalScreen();

struct termios ttySetRaw();

bool isInRect(int row, int col, Rect *rect);

void renderCell(VTermScreenCell cell);

#endif
