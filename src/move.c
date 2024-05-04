#include "move.h"
#include "session.h"

#define is_within_rect(r, p) \
  (p.row >= r.start_row && p.row < r.end_row && p.col >= r.start_col && p.col < r.end_col)

int move_left(Window *w) {
  Pane *currentPane = &w->panes[w->current_pane];
  int col = currentPane->col;
  int row = currentPane->row;
  col--;

  while (true) {
    if (col < 0) {
      col = w->width - 1;
    }
    for (int r = row; r < row + currentPane->height; r++) {
      for (int i = 0; i < w->pane_count; i++) {
        Pane *pane = &w->panes[i];
        VTermRect rect = {.start_col = pane->col,
                          .start_row = pane->row,
                          .end_col = pane->col + pane->width,
                          .end_row = pane->row + pane->height};
        VTermPos pos = {.col = col, .row = r};
        if (is_within_rect(rect, pos)) {
          return i;
        }
      }
    }
    col--;
  }
}

int move_right(Window *w) {
  Pane *currentPane = &w->panes[w->current_pane];
  int col = currentPane->col;
  int row = currentPane->row;
  col += currentPane->width;

  while (true) {
    if (col >= w->width) {
      col = 0;
    }
    for (int r = row; r < row + currentPane->height; r++) {
      for (int i = 0; i < w->pane_count; i++) {
        Pane *pane = &w->panes[i];
        VTermRect rect = {.start_col = pane->col,
                          .start_row = pane->row,
                          .end_col = pane->col + pane->width,
                          .end_row = pane->row + pane->height};
        VTermPos pos = {.col = col, .row = r};
        if (is_within_rect(rect, pos)) {
          return i;
        }
      }
    }
    col++;
  }
}

int move_up(Window *w) {
  Pane *currentPane = &w->panes[w->current_pane];
  int col = currentPane->col;
  int row = currentPane->row;
  row--;

  while (true) {
    if (row < 0) {
      row = w->height - 1;
    }
    for (int c = col; c < col + currentPane->width; c++) {
      for (int i = 0; i < w->pane_count; i++) {
        Pane *pane = &w->panes[i];
        VTermRect rect = {.start_col = pane->col,
                          .start_row = pane->row,
                          .end_col = pane->col + pane->width,
                          .end_row = pane->row + pane->height};
        VTermPos pos = {.col = c, .row = row};
        if (is_within_rect(rect, pos)) {
          return i;
        }
      }
    }
    row--;
  }
}

int move_down(Window *w) {
  Pane *currentPane = &w->panes[w->current_pane];
  int col = currentPane->col;
  int row = currentPane->row;
  row += currentPane->height;

  while (true) {
    if (row >= w->height) {
      row = 0;
    }
    for (int c = col; c < col + currentPane->width; c++) {
      for (int i = 0; i < w->pane_count; i++) {
        Pane *pane = &w->panes[i];
        VTermRect rect = {.start_col = pane->col,
                          .start_row = pane->row,
                          .end_col = pane->col + pane->width,
                          .end_row = pane->row + pane->height};
        VTermPos pos = {.col = c, .row = row};
        if (is_within_rect(rect, pos)) {
          return i;
        }
      }
    }
    row++;
  }
}

int move_active_pane(Direction d, Window *w) {
  switch (d) {
  case LEFT:
    return move_left(w);
  case RIGHT:
    return move_right(w);
  case UP:
    return move_up(w);
  case DOWN:
    return move_down(w);
  default:
    return -1;
  }
}
