#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

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

bool is_in_rect(int row, int col, int rowRect, int colRect, int height,
                int width) {
  return row >= rowRect && row < rowRect + height && col >= colRect &&
         col < colRect + width;
}

void write_position(int row, int col) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);
  buf_write(buf, n);
}

void clear_style() {
  bzero(&neotmux->prevCell, sizeof(neotmux->prevCell));
  buf_write("\033[0m", 4); // Reset style
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

bool is_bordering_active_pane(int row, int col, Window *window) {
  Pane *pane = &window->panes[window->current_pane];

  for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
      if (is_in_rect(row, col, pane->row + y, pane->col + x, pane->height,
                     pane->width)) {
        return true;
      }
    }
  }

  return false;
}

bool is_in_pane(int row, int col, Window *window) {
  for (int i = 0; i < window->pane_count; i++) {
    Pane *pane = &window->panes[i];
    if (is_in_rect(row, col, pane->row, pane->col, pane->height, pane->width)) {
      return true;
    }
  }

  return false;
}

void write_border_character(int row, int col, Window *window) {
  if (is_in_pane(row, col, window)) {
    buf_write("\033[1C", 4);
    return;
  }

  bool r = is_border(row, col + 1, window);
  bool l = is_border(row, col - 1, window);
  bool d = is_border(row + 1, col, window);
  bool u = is_border(row - 1, col, window);

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

void write_cursor_position() {
  VTermPos cursorPos;
  Session *currentSession = &neotmux->sessions[neotmux->current_session];
  Window *currentWindow =
      &currentSession->windows[currentSession->current_window];
  if (currentWindow->current_pane != -1) {
    Pane *currentPane = &currentWindow->panes[currentWindow->current_pane];
    VTermState *state = vterm_obtain_state(currentPane->process.vt);
    vterm_state_get_cursorpos(state, &cursorPos);
    cursorPos.row += currentPane->row;
    cursorPos.col += currentPane->col;

    if (neotmux->barPos == BAR_TOP) {
      cursorPos.row++;
    }

    write_position(cursorPos.row + 1, cursorPos.col + 1);
  }
}

void write_cursor_style() {
  Session *currentSession = &neotmux->sessions[neotmux->current_session];
  Window *currentWindow =
      &currentSession->windows[currentSession->current_window];
  if (currentWindow->current_pane != -1) {
    Pane *currentPane = &currentWindow->panes[currentWindow->current_pane];
    if (currentPane->process.cursor_visible) {
      buf_write("\033[?25h", 6); // Show cursor
    } else {
      buf_write("\033[?25l", 6); // Hide cursor
    }

    switch (currentPane->process.cursor_shape) {
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

void draw_pane(Pane *pane, Window *window) {
  clear_style();
  for (int row = pane->row; row < pane->row + pane->height; row++) {
    write_position(row + 1, pane->col + 1);
    for (int col = pane->col; col < pane->col + pane->width; col++) {
      VTermPos pos;
      pos.row = row - pane->row;
      pos.col = col - pane->col;
      VTermScreen *vts = pane->process.vts;

      VTermScreenCell cell = {0};
      vterm_screen_get_cell(vts, pos, &cell);
      render_cell(cell);
    }
  }
}

void draw_borders(Window *window) {
  buf_color(2);
  for (int row = 0; row < window->height; row++) {
    write_position(row + 1, 1);
    for (int col = 0; col < window->width; col++) {
      write_border_character(row, col, window);
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
          render_cell(cell);
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

void render_bar(int fd, int rows, int cols) {
  neotmux->bb.n = 0;

  if (neotmux->barPos == BAR_TOP) {
    write_position(1, 1);
    write_status_bar(cols);
  } else if (neotmux->barPos == BAR_BOTTOM) {
    write_position(rows + 1, 1);
    write_status_bar(cols);
  }

  write_cursor_position();
  write_cursor_style();

  write(fd, neotmux->bb.buffer, neotmux->bb.n);
}

void render_screen(int fd, int rows, int cols) {
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

    char *data = read_file("dosascii");
    vterm_input_write(vt, data, strlen(data));
    free(data);
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

  Session *currentSession = &neotmux->sessions[neotmux->current_session];
  Window *currentWindow =
      &currentSession->windows[currentSession->current_window];
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
