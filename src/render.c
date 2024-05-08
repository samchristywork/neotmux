#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "border.h"
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

#define is_in_rect(row, col, rowRect, colRect, height, width)                  \
  (row >= rowRect && row < rowRect + height && col >= colRect &&               \
   col < colRect + width)

#define write_position(row, col)                                               \
  char buf[32];                                                                \
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);                          \
  buf_write(buf, n);

void draw_cell(VTermScreenCell cell) { render_cell(cell); }

void clear_style() {
  bzero(&neotmux->prevCell, sizeof(neotmux->prevCell));
  buf_write("\033[0m", 4); // Reset style
}

void draw_pane(Pane *pane, Window *window) {
  clear_style();
  for (int row = pane->row; row < pane->row + pane->height; row++) {
    write_position(row + 1, pane->col + 1);
    for (int col = pane->col; col < pane->col + pane->width; col++) {
      VTermPos pos;
      pos.row = row - pane->row;
      pos.col = col - pane->col;
      VTermScreen *vts = pane->process->vts;

      VTermScreenCell cell = {0};
      vterm_screen_get_cell(vts, pos, &cell);
      draw_cell(cell);
    }
  }
}

void draw_row(int paneRow, int windowRow, Pane *pane) {
  write_position(windowRow + 1, pane->col + 1);

  for (int col = pane->col; col < pane->col + pane->width; col++) {
    VTermPos pos;
    pos.row = paneRow;
    pos.col = col - pane->col;
    VTermScreen *vts = pane->process->vts;

    VTermScreenCell cell = {0};
    vterm_screen_get_cell(vts, pos, &cell);
    draw_cell(cell);
  }
}

VTermScreenCell *get_history_cell(ScrollBackLines scrollback, int row,
                                  int col) {
  row = scrollback.size - row;
  if (row < 0 || row >= scrollback.size) {
    return NULL;
  }

  ScrollBackLine *line = &scrollback.buffer[row];
  if (col < 0 || col >= line->cols) {
    return NULL;
  }

  return &line->cells[col];
}

void draw_history_row(int paneRow, int windowRow, Pane *pane) {
  write_position(windowRow + 1, pane->col + 1);

  for (int col = pane->col; col < pane->col + pane->width; col++) {
    ScrollBackLines sb = pane->process->scrollback;
    VTermScreenCell *cell = get_history_cell(sb, paneRow, col - pane->col);
    if (cell) {
      draw_cell(*cell);
    } else {
      buf_write(" ", 1);
    }
  }
}

void draw_bar(Pane *pane, Window *window) {
  int row = pane->row + pane->height - 1;
  write_position(row + 1, pane->col + 1);
  char *str = get_pane_bar(pane);
  for (int col = 0; col < pane->width; col++) {
    VTermScreenCell cell = {0};
    if (col < strlen(str)) {
      cell.chars[0] = str[col];
    } else {
      cell.chars[0] = ' ';
    }
    cell.bg.type = VTERM_COLOR_DEFAULT_BG;
    cell.bg.indexed.idx = 0;
    cell.fg.type = VTERM_COLOR_INDEXED;
    cell.fg.indexed.idx = 7;
    draw_cell(cell);
  }
  free(str);
}

void draw_pane(Pane *pane, Window *window) {
  clear_style();
  for (int row = 0; row < pane->height; row++) {
    int windowRow = pane->row + row;
    if (windowRow < 0 || windowRow >= window->height) {
      continue;
    }

    int scroll = pane->process->scrolloffset;
    int paneRow = row + scroll;
    if (paneRow >= 0) {
      draw_row(paneRow, windowRow, pane);
    }
  }
}

void draw_floating_window(FloatingWindow *floatingWindow, Window *window) {
  if (floatingWindow->visible) {
    clear_style();
    for (int row = 0; row < floatingWindow->height; row++) {
      write_position(row + 1, 1);
      for (int col = 0; col < floatingWindow->width; col++) {
        if (is_in_rect(row, col, 0, 0, floatingWindow->height,
                       floatingWindow->width)) {
          VTermScreenCell cell = {0};
          VTermPos pos;
          pos.row = row;
          pos.col = col;
          VTermScreen *vts = floatingWindow->vts;
          vterm_screen_get_cell(vts, pos, &cell);
          cell.bg.type = VTERM_COLOR_INDEXED;
          cell.bg.indexed.idx = 8;
          draw_cell(cell);
        }
      }
    }
  }
}

char *read_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = malloc(length + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }

  fread(buffer, 1, length, file);
  fclose(file);
  buffer[length] = '\0';

  return buffer;
}

void render_screen(int fd) {
  if (!floatingWindow) {
    floatingWindow = malloc(sizeof(FloatingWindow));
    floatingWindow->height = 43;
    floatingWindow->width = 80;
    floatingWindow->visible = false;

    VTerm *vt;
    VTermScreen *vts;
    vt = vterm_new(floatingWindow->height, floatingWindow->width);
    vts = vterm_obtain_screen(vt);

    static VTermScreenCallbacks callbacks;
    callbacks.damage = NULL;
    callbacks.moverect = NULL;
    callbacks.movecursor = NULL;
    callbacks.settermprop = NULL;
    callbacks.bell = NULL;
    callbacks.resize = NULL;
    callbacks.sb_pushline = NULL;
    callbacks.sb_popline = NULL;

    vterm_set_utf8(vt, 1);
    vterm_screen_reset(vts, 1);
    vterm_screen_set_callbacks(vts, &callbacks, NULL);

    // char *data = read_file("dosascii");
    char *data = "Test";
    vterm_input_write(vt, data, strlen(data));
    // free(data);
    floatingWindow->vt = vt;
    floatingWindow->vts = vts;
  }

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

  neotmux->bb.n = 0;
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
    draw_borders(currentWindow);
  }

  draw_floating_window(floatingWindow, currentWindow);

  write(fd, neotmux->bb.buffer, neotmux->bb.n);
}

void render(int fd, RenderType type) {
  if (type == RENDER_SCREEN) {
    render_screen(fd);
  } else if (type == RENDER_BAR) {
    render_bar(fd);
  }
}
