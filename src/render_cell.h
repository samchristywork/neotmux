#ifndef RENDER_CELL_H
#define RENDER_CELL_H

#include <vterm.h>

void render_cell(VTermScreenCell *cell, int row, int col);
int fill_utf8(long codepoint, char *str);

#endif
