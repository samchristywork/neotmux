#include "render_row.h"

extern Neotmux *neotmux;

#define clear_style()                                                          \
  bzero(&neotmux->prevCell, sizeof(neotmux->prevCell));                        \
  buf_write("\033[0m", 4);

void draw_pane(Pane *pane, Window *window) {
  if (pane->process->cursor.mouse_active != VTERM_PROP_MOUSE_NONE) {
    pane->selection.active = false;
  }
  clear_style();
  for (int row = 0; row < pane->height; row++) {
    int windowRow = pane->row + row;
    if (windowRow < 0 || windowRow >= window->height) {
      continue;
    }

    draw_row(row, windowRow, pane, window);
  }
  // draw_bar(pane, window);
}
