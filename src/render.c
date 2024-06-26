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
#include "render_pane.h"
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

#define write_position(row, col)                                               \
  char buf[32];                                                                \
  int n = snprintf(buf, 32, "\033[%d;%dH", row, col);                          \
  buf_write(buf, n);

#define clear_style()                                                          \
  bzero(&neotmux->prevCell, sizeof(neotmux->prevCell));                        \
  buf_write("\033[0m", 4);

// TODO: Fix top bar
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

void handle_post_render(int fd) {
  buf_write("\033[1;1H", 6);
  buf_write("\033[0m", 4);

  lua_getglobal(neotmux->lua, "post_render");
  if (lua_isfunction(neotmux->lua, -1)) {
    Window *currentWindow = get_current_window(neotmux);
    lua_pushinteger(neotmux->lua, currentWindow->width);
    lua_pushinteger(neotmux->lua, currentWindow->height);
    lua_call(neotmux->lua, 2, 1);
    if (lua_isstring(neotmux->lua, -1)) {
      const char *result = lua_tostring(neotmux->lua, -1);
      buf_write(result, strlen(result));
    }
    lua_pop(neotmux->lua, 1);
  } else {
    WRITE_LOG(LOG_WARN, fd, "post_render is not a function");
  }
}

void render(int fd, RenderType type) {
  neotmux->bb.n = 0 + sizeof(uint64_t);

  if (type == RENDER_SCREEN) {
    int before = neotmux->bb.n;
    render_screen();
    Window *currentWindow = get_current_window(neotmux);
    int x = currentWindow->width;
    int y = currentWindow->height;
    int n = neotmux->bb.n - before;
    float r = (float)n / (x * y);
    WRITE_LOG(LOG_PERF, fd, "Render (%dx%d). Writing %d bytes (%f bytes/cell)",
              x, y, n, r);

    handle_post_render(fd);
  } else if (type == RENDER_BAR) {
    render_bar(fd);
  }

  bool show_cursor = write_cursor_position();
  if (show_cursor) {
    write_cursor_style();
  }

  memcpy(neotmux->bb.buffer, &neotmux->bb.n, sizeof(uint64_t));
  write(fd, neotmux->bb.buffer, neotmux->bb.n);
}
