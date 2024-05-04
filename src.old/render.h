#ifndef RENDER_H
#define RENDER_H

#include <vterm.h>

typedef struct Pane {
  int process;
  VTerm *vt;
  VTermScreen *vts;
  int width;
  int height;
  int col;
  int row;
  bool closed;
} Pane;

void renderScreen(int fifo, Pane *panes, int nPanes, int activeTerm, int rows,
                  int cols);

#endif
