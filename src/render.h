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
  bool closed;
} Window;

struct termios ttySetRaw();

void renderScreen(int fifo, Window *windows, int nWindows, int activeTerm,
                  int rows, int cols);

#endif
