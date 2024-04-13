#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include <vterm.h>

typedef struct Window {
  int process;
  VTerm *vt;
  VTermScreen *vts;
  int width;
  int height;
  int col;
  int row;
} Window;

void makeCursorInvisible();

void makeCursorVisible();

void alternateScreen();

void normalScreen();

struct termios ttySetRaw();

void renderScreen(Window *windows, int activeTerm, int rows, int cols);

#endif
