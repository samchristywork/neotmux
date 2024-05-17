#define _GNU_SOURCE

#include <fcntl.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "border.h"
#include "log.h"
#include "render.h"
#include "render_cell.h"
#include "session.h"
#include "statusbar.h"

extern Neotmux *neotmux;

// TODO: Only show floating window on first call of render_screen
// Maybe that should happen on the client instead
typedef struct FloatingWindow {
  int height;
  int width;
  bool visible;
  VTerm *vt;
  VTermScreen *vts;
} FloatingWindow;

FloatingWindow *floatingWindow = NULL;

bool renderBorders = true;

#define is_in_rect(row, col, rowRect, colRect, height, width)                  \
  (row >= rowRect && row < rowRect + height && col >= colRect &&               \
   col < colRect + width)

#define write_position(row, col)                                               \
  char buf[32];                                                                \
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);                          \
  buf_write(buf, n);

void draw_cell(VTermScreenCell *cell, int row, int col) {
  render_cell(cell, row, col);
}

void clear_style() {
  bzero(&neotmux->prevCell, sizeof(neotmux->prevCell));
  buf_write("\033[0m", 4); // Reset style
}

bool compare_cells(VTermScreenCell *a, VTermScreenCell *b) {
  int ret = memcmp(a, b, sizeof(VTermScreenCell));
  return ret != 0;
}

void draw_history_row(int paneRow, int windowRow, Pane *pane);

// TODO: Add module
bool is_between_cells(VTermPos pos, VTermPos start, VTermPos end) {
  start.col--;
  start.row--;
  end.row--;

  if (pos.row > start.row && pos.row < end.row) {
    return true;
  }

  if (pos.row < start.row || pos.row > end.row) {
    return false;
  }

  if (pos.row == start.row && pos.col < start.col) {
    return false;
  }

  if (pos.row == end.row && pos.col >= end.col) {
    return false;
  }

  return true;
}

bool is_within_selection(Selection selection, VTermPos pos) {
  if (!selection.active) {
    return false;
  }

  VTermPos start = selection.start;
  VTermPos end = selection.end;

  if (is_between_cells(pos, start, end)) {
    return true;
  }

  if (is_between_cells(pos, end, start)) {
    return true;
  }

  return false;
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

      int idx = col + windowRow * currentWindow->width;
      if (compare_cells(&cell, &prevCells[idx])) {
        prevCells[idx] = cell;
        draw_cell(&cell, windowRow + 1, col + 1);
        styleChanged = true; // TODO: Investigate this
      }
    } else {
      write_position(windowRow + 1, col + 1);
      buf_write(" ", 1);
      // draw_history_row(-paneRow, windowRow, pane);
    }
  }

  if (styleChanged) {
    clear_style();
  }
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

// TODO: Add scrollback module
void draw_history_row(int paneRow, int windowRow, Pane *pane) {
  write_position(windowRow + 1, pane->col + 1);
  for (int col = pane->col; col < pane->col + pane->width; col++) {
    ScrollBackLines sb = pane->process->scrollback;
    VTermScreenCell *cell = get_history_cell(sb, paneRow, col - pane->col);
    if (cell) {
      draw_cell(cell, windowRow + 1, pane->col + 1);
    } else {
      buf_write(" ", 1);
    }
  }
}

// int get_resource_usage(int pid) {
//   struct rusage usage;
//   getrusage(RUSAGE_CHILDREN, &usage);
//   return usage.ru_maxrss;
// }
//
// char *get_pane_bar(Pane *pane) {
//   int usage = get_resource_usage(pane->process->pid);
//
//   char *str = malloc(pane->width);
//   int n = snprintf(str, pane->width, "%s %d", pane->process->name, usage);
//   return str;
// }
//
// void draw_bar(Pane *pane, Window *window) {
//   int row = pane->row + pane->height - 1;
//   write_position(row + 1, pane->col + 1);
//   char *str = get_pane_bar(pane);
//   for (int col = 0; col < pane->width; col++) {
//     VTermScreenCell cell = {0};
//     if (col < strlen(str)) {
//       cell.chars[0] = str[col];
//     } else {
//       cell.chars[0] = ' ';
//     }
//     cell.bg.type = VTERM_COLOR_DEFAULT_BG;
//     cell.bg.indexed.idx = 0;
//     cell.fg.type = VTERM_COLOR_INDEXED;
//     cell.fg.indexed.idx = 7;
//     draw_cell(cell);
//   }
//   free(str);
// }

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

void make_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}


void render_screen() {
  if (neotmux->barPos == BAR_NONE) {
    neotmux->barPos = BAR_BOTTOM;

    lua_getglobal(neotmux->lua, "bar_position");
    if (!lua_isstring(neotmux->lua, -1)) {
      lua_pop(neotmux->lua, 1);
    } else {
      if (strcmp(lua_tostring(neotmux->lua, -1), "top") == 0) {
        neotmux->barPos = BAR_TOP;
      }
      lua_pop(neotmux->lua, 1);
    }
  }

  if (neotmux->bb.buffer == NULL) {
    neotmux->bb.buffer = malloc(100);
    neotmux->bb.capacity = 100;
  }

  buf_write("\033[?25l", 6); // Hide cursor

  Window *currentWindow = get_current_window(neotmux);
  if (currentWindow->zoom != -1) {
    Pane *pane = &currentWindow->panes[currentWindow->zoom];
    draw_pane(pane, currentWindow);
  } else {
    for (int k = 0; k < currentWindow->pane_count; k++) {
      Pane *pane = &currentWindow->panes[k];
      draw_pane(pane, currentWindow);
    }

    if (renderBorders) {
      renderBorders = false;
      if (neotmux->barPos == BAR_BOTTOM) {
        draw_borders(currentWindow, 0);
      } else {
        draw_borders(currentWindow, 1);
      }
    }
  }
}

#define write_position(row, col)                                               \
  char buf[32];                                                                \
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);                          \
  buf_write(buf, n);

bool write_cursor_position() {
  VTermPos cursorPos;
  Pane *currentPane = get_current_pane(neotmux);
  if (currentPane) {
    VTermState *state = vterm_obtain_state(currentPane->process->vt);
    vterm_state_get_cursorpos(state, &cursorPos);
    cursorPos.row += currentPane->row - currentPane->process->scrolloffset;
    cursorPos.col += currentPane->col;

    if (neotmux->barPos == BAR_TOP) {
      cursorPos.row++;
    }

    if (cursorPos.row < 0 ||
        cursorPos.row >= currentPane->height + currentPane->row) {
      buf_write("\033[?25l", 6); // Hide cursor
      return false;
    } else {
      write_position(cursorPos.row + 1, cursorPos.col + 1);
      return true;
    }
  }

  return false;
}

void write_cursor_style() {
  Pane *currentPane = get_current_pane(neotmux);
  if (currentPane) {
    if (currentPane->process->cursor.visible) {
      buf_write("\033[?25h", 6); // Show cursor
    } else {
      buf_write("\033[?25l", 6); // Hide cursor
    }

    switch (currentPane->process->cursor.shape) {
    case VTERM_PROP_CURSORSHAPE_BLOCK:
      buf_write("\033[0 q", 6); // Block cursor
      break;
    case VTERM_PROP_CURSORSHAPE_UNDERLINE:
      buf_write("\033[3 q", 6); // Underline cursor
      break;
    case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
      buf_write("\033[5 q", 6); // Vertical bar cursor
      break;
    default:
      break;
    }
  }
}

void render(int fd, RenderType type) {
  if (type == RENDER_SCREEN) {
    int before = neotmux->bb.n;
    render_screen();
    Window *currentWindow = get_current_window(neotmux);
    int x = currentWindow->width;
    int y = currentWindow->height;
    int n = neotmux->bb.n - before;
    float r = (float)n / (x * y);
    WRITE_LOG(fd, "Render (%dx%d). Writing %d bytes (%f bytes/cell)", x, y, n,
              r);
  } else if (type == RENDER_BAR) {
    render_bar(fd);
  }

  bool show_cursor = write_cursor_position();
  if (show_cursor) {
    write_cursor_style();
  }

  write(fd, neotmux->bb.buffer, neotmux->bb.n);
  neotmux->bb.n = 0;
}
