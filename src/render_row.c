#include "clipboard.h"
#include "render_cell.h"
#include "session.h"

extern Neotmux *neotmux;
extern bool renderBorders; // TODO: Remove this

#define write_position(row, col)                                               \
  char buf[32];                                                                \
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);                          \
  buf_write(buf, n);

#define clear_style()                                                          \
  bzero(&neotmux->prevCell, sizeof(neotmux->prevCell));                        \
  buf_write("\033[0m", 4);

bool compare_cells(VTermScreenCell *a, VTermScreenCell *b) {
  int ret = memcmp(a, b, sizeof(VTermScreenCell));
  return ret != 0;
}

void draw_cell(VTermScreenCell *cell, int row, int col) {
  render_cell(cell, row, col);
}

VTermScreenCell *get_history_cell(ScrollBackLines scrollback, int row,
                                  int col) {
  row = scrollback.size - row;
  if ((size_t)row < 0 || (size_t)row >= scrollback.size) {
    return NULL;
  }

  ScrollBackLine *line = &scrollback.buffer[row];
  if (col < 0 || col >= line->cols) {
    return NULL;
  }

  return &line->cells[col];
}

void draw_history_row(int paneRow, int windowRow, Pane *pane) {
  for (int col = pane->col; col < pane->col + pane->width; col++) {
    ScrollBackLines sb = pane->process->scrollback;
    VTermScreenCell *cell = get_history_cell(sb, paneRow, col - pane->col);
    if (cell) {
      draw_cell(cell, windowRow + 1, col + 1);
    } else {
      VTermScreenCell cell = {0};
      cell.chars[0] = '.';
      cell.width = 1;
      cell.fg.type = VTERM_COLOR_INDEXED;
      cell.fg.indexed.idx = 7;
      draw_cell(&cell, windowRow + 1, col + 1);
    }
  }
}

void draw_row(int row, int windowRow, Pane *pane, Window *currentWindow) {
  if (neotmux->barPos == BAR_TOP) {
    windowRow += 1;
  }

  static VTermScreenCell *prevCells = NULL;
  if (prevCells == NULL) {
    prevCells = malloc(sizeof(VTermScreenCell));
  }

  // TODO: Simplify all of this down to just currentWindow->rerender
  {
    static int width = 0;
    static int height = 0;
    static Window *window = NULL;
    static int pane_count = 0;
    static Layout layout = 0;
    static int current_pane = 0;
    static int zoom = -1;
    if (width != currentWindow->width || height != currentWindow->height ||
        window != currentWindow || currentWindow->pane_count != pane_count ||
        currentWindow->layout != layout || currentWindow->rerender ||
        currentWindow->current_pane != current_pane ||
        currentWindow->zoom != zoom) {
      int n = currentWindow->width * currentWindow->height;
      prevCells = realloc(prevCells, n * sizeof(VTermScreenCell));
      bzero(prevCells, n * sizeof(VTermScreenCell));
      width = currentWindow->width;           // Needed when resizing window
      height = currentWindow->height;         // Needed when resizing window
      window = currentWindow;                 // Needed when deleting a window
      pane_count = currentWindow->pane_count; // Needed when deleting a pane
      layout = currentWindow->layout;         // Needed when changing layout
      current_pane = currentWindow->current_pane; // Needed to update border
      zoom = currentWindow->zoom;                 // Needed when changing zoom
      currentWindow->rerender = false;
      renderBorders = true;
    }
  }

  bool styleChanged = false;
  int paneRow = row + pane->process->scrolloffset;
  for (int col = pane->col; col < pane->col + pane->width; col++) {
    VTermPos pos;
    pos.row = paneRow;
    pos.col = col - pane->col;
    VTermScreen *vts = pane->process->vts;

    if (paneRow >= pane->height) {
      VTermScreenCell cell = {0};
      cell.chars[0] = 'x';
      cell.bg.type = VTERM_COLOR_DEFAULT_BG;
      cell.bg.indexed.idx = 0;
      cell.fg.type = VTERM_COLOR_INDEXED;
      cell.fg.indexed.idx = 7;
      cell.width = 1;

      draw_cell(&cell, windowRow + 1, col + 1);
    } else if (paneRow >= 0) {
      VTermScreenCell cell = {0};
      vterm_screen_get_cell(vts, pos, &cell);

      if (is_within_selection(pane->selection, pos)) {
        cell.attrs.reverse = 1;
      }

      // TODO: Fold this logic into draw_cell
      int idx = col + windowRow * currentWindow->width;
      if (compare_cells(&cell, &prevCells[idx])) {
        prevCells[idx] = cell;
        draw_cell(&cell, windowRow + 1, col + 1);
        styleChanged = true; // TODO: Investigate this
      }
    } else {
      write_position(windowRow + 1, col + 1);
      buf_write(" ", 1);
      draw_history_row(-paneRow, windowRow, pane);
    }
  }

  if (styleChanged) {
    clear_style();
  }
}
