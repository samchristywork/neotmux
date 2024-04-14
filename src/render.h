#ifndef RENDER_H
#define RENDER_H

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

void renderScreen(int fifo, Window *windows, int nWindows, int activeTerm,
                  int rows, int cols);

#endif
