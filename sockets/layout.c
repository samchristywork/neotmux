#include <sys/ioctl.h>

#include "layout.h"

void update_layout(Window *w) {
  for (int i = 0; i < w->pane_count; i++) {
    vterm_set_size(w->panes[i].process.vt, w->panes[i].height,
                   w->panes[i].width);

    struct winsize ws;
    ws.ws_row = w->panes[i].height;
    ws.ws_col = w->panes[i].width;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    if (ioctl(w->panes[i].process.fd, TIOCSWINSZ, &ws) < 0) {
      exit(EXIT_FAILURE);
    }
  }
}

void even_horizontal_layout(Window *w) {
  Pane *panes = w->panes;

  int remaining = w->width - w->pane_count + 1;
  int col = 0;

  for (int i = 0; i < w->pane_count; i++) {
    panes[i].row = 0;
    panes[i].col = col;
    panes[i].height = w->height;
    panes[i].width = remaining / (w->pane_count - i);

    col += panes[i].width + 1;
    remaining -= panes[i].width;
  }

  update_layout(w);
}

void even_vertical_layout(Window *w) {
  Pane *panes = w->panes;

  int remaining = w->height - w->pane_count + 1;
  int row = 0;

  for (int i = 0; i < w->pane_count; i++) {
    panes[i].row = row;
    panes[i].col = 0;
    panes[i].height = remaining / (w->pane_count - i);
    panes[i].width = w->width;

    row += panes[i].height + 1;
    remaining -= panes[i].height;
  }

  update_layout(w);
}

void main_horizontal_layout(Window *w) {
  Pane *panes = w->panes;

  if (w->pane_count <= 1) {
    even_vertical_layout(w);
    return;
  }

  panes[0].row = 0;
  panes[0].col = 0;
  panes[0].height = w->height / 2;
  panes[0].width = w->width;

  int remaining = w->width;
  for (int i = 1; i < w->pane_count; i++) {
    panes[i].row = w->height / 2 + 1;
    panes[i].col = w->width - remaining;
    panes[i].height = w->height - panes[i].row;
    panes[i].width = remaining / (w->pane_count - i);

    remaining -= panes[i].width;
    remaining--;
  }

  update_layout(w);
}

void main_vertical_layout(Window *w) {
  Pane *panes = w->panes;

  if (w->pane_count <= 1) {
    even_horizontal_layout(w);
    return;
  }

  panes[0].row = 0;
  panes[0].col = 0;
  panes[0].height = w->height;
  panes[0].width = w->width / 2;

  int remaining = w->height;
  for (int i = 1; i < w->pane_count; i++) {
    panes[i].row = w->height - remaining;
    panes[i].col = w->width / 2 + 1;
    panes[i].height = remaining / (w->pane_count - i);
    panes[i].width = w->width - panes[i].col;

    remaining -= panes[i].height;
    remaining--;
  }

  update_layout(w);
}

void tiled_layout(Window *w) {
  Pane *panes = w->panes;

  int rows = 1;
  int cols = 1;

  while (rows * cols < w->pane_count) {
    if (w->width / (cols + 1) > w->height / (rows + 1)) {
      cols++;
    } else {
      rows++;
    }
  }

  int row = 0;
  int col = 0;
  int remaining = w->pane_count;

  for (int i = 0; i < w->pane_count; i++) {
    panes[i].row = row;
    panes[i].col = col;
    panes[i].height = w->height / rows;
    panes[i].width = w->width / cols;

    col += panes[i].width + 1;
    remaining--;

    if (remaining % cols == 0) {
      row += panes[i].height + 1;
      col = 0;
    }
  }

  update_layout(w);
}

void calculate_layout(Window *window) {
  if (window->zoom != -1) {
    window->panes[window->zoom].row = 0;
    window->panes[window->zoom].col = 0;
    window->panes[window->zoom].height = window->height;
    window->panes[window->zoom].width = window->width;
    update_layout(window);
  } else {
    switch (window->layout) {
    case LAYOUT_DEFAULT:
      // TODO: Implement default layout
      break;
    case LAYOUT_EVEN_HORIZONTAL:
      even_horizontal_layout(window);
      break;
    case LAYOUT_EVEN_VERTICAL:
      even_vertical_layout(window);
      break;
    case LAYOUT_MAIN_HORIZONTAL:
      main_horizontal_layout(window);
      break;
    case LAYOUT_MAIN_VERTICAL:
      main_vertical_layout(window);
      break;
    case LAYOUT_TILED:
      tiled_layout(window);
      break;
    default:
      break;
    }
  }
}