#include <stdbool.h>
#include <stdio.h>

#include "session.h"

extern Neotmux *neotmux;

#define is_in_rect(row, col, rowRect, colRect, height, width)                  \
  (row >= rowRect && row < rowRect + height && col >= colRect &&               \
   col < colRect + width)

#define write_position(row, col)                                               \
  char buf[32];                                                                \
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);                          \
  buf_write(buf, n);

bool is_in_pane(int row, int col, Window *window) {
  for (int i = 0; i < window->pane_count; i++) {
    Pane *pane = &window->panes[i];
    if (is_in_rect(row, col, pane->row, pane->col, pane->height, pane->width)) {
      return true;
    }
  }

  return false;
}

bool is_border(int row, int col, Window *window) {
  for (int i = 0; i < window->pane_count; i++) {
    Pane *pane = &window->panes[i];
    if (is_in_rect(row, col, pane->row, pane->col, pane->height, pane->width)) {
      return false;
    }
  }

  bool outsideAllPanes = true;
  for (int i = 0; i < window->pane_count; i++) {
    Pane *pane = &window->panes[i];
    if (is_in_rect(row, col, pane->row - 1, pane->col - 1, pane->height + 2,
                   pane->width + 2)) {
      outsideAllPanes = false;
      break;
    }
  }

  if (outsideAllPanes) {
    return false;
  }

  return true;
}

bool borders_active_pane(int row, int col, Window *window) {
  for (int i = 0; i < window->pane_count; i++) {
    Pane *pane = &window->panes[i];
    if (window->current_pane == i) {
      if (is_in_rect(row, col, pane->row - 1, pane->col - 1, pane->height + 2,
                     pane->width + 2)) {
        return true;
      }
    }
  }

  return false;
}

int lastColor;
void write_border_character(int row, int col, Window *window) {
  if (is_in_pane(row, col, window)) {
    buf_write("\033[1C", 4);
    return;
  }

  bool r = is_border(row, col + 1, window);
  bool l = is_border(row, col - 1, window);
  bool d = is_border(row + 1, col, window);
  bool u = is_border(row - 1, col, window);

  if (u || d || l || r) {
    if (borders_active_pane(row, col, window)) {
      if (lastColor != 2) {
        buf_color(2);
      }
    } else {
      if (lastColor != 8) {
        buf_color(8);
      }
    }
  }

  if (u && d && l && r) {
    buf_write("┼", 3);
  } else if (u && d && l && !r) {
    buf_write("┤", 3);
  } else if (u && d && !l && r) {
    buf_write("├", 3);
  } else if (u && d && !l && !r) {
    buf_write("│", 3);
  } else if (u && !d && l && r) {
    buf_write("┴", 3);
  } else if (u && !d && l && !r) {
    buf_write("┘", 3);
  } else if (u && !d && !l && r) {
    buf_write("└", 3);
  } else if (!u && d && l && r) {
    buf_write("┬", 3);
  } else if (!u && d && l && !r) {
    buf_write("┐", 3);
  } else if (!u && d && !l && r) {
    buf_write("┌", 3);
  } else if (!u && !d && l && r) {
    buf_write("─", 3);
  } else {
    buf_write(" ", 1);
  }
}

// TODO: Ensure that we color active borders
void draw_borders(Window *window) {
  lastColor = 0;
  for (int row = 0; row < window->height; row++) {
    write_position(row + 1, 1);
    for (int col = 0; col < window->width; col++) {
      write_border_character(row, col, window);
    }
  }
}
