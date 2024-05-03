#ifndef SESSION_H
#define SESSION_H

#include <lua5.4/lua.h>
#include <vterm.h>

typedef enum Layout {
  LAYOUT_DEFAULT = 0,
  LAYOUT_EVEN_HORIZONTAL,
  LAYOUT_EVEN_VERTICAL,
  LAYOUT_MAIN_HORIZONTAL,
  LAYOUT_MAIN_VERTICAL,
  LAYOUT_TILED,
  LAYOUT_CUSTOM,
} Layout;

typedef struct Process {
  char *name;
  int pid;
  int fd;
  VTerm *vt;
  VTermScreen *vts;
  bool closed;
  bool cursor_visible;
  int cursor_shape;
  int mouse;
} Process;

typedef struct Pane {
  int row;
  int col;
  int width;
  int height;
  Process process;
} Pane;

typedef struct Window {
  Pane *panes;
  char *title;
  int pane_count;
  int current_pane;
  int width;
  int height;
  Layout layout;
  int zoom;
} Window;

typedef struct Session {
  Window *windows;
  char *title;
  int window_count;
  int current_window;
} Session;

typedef struct BackBuffer {
  char *buffer;
  int n;
  int capacity;
} BackBuffer;

typedef struct Neotmux {
  Session *sessions;
  int session_count;
  int current_session;
  pthread_mutex_t mutex;
  lua_State *lua;
  BackBuffer bb;
  VTermScreenCell prevCell;
} Neotmux;

#define bb_write(buf, count)                                                   \
  do {                                                                         \
    if (neotmux->bb.n + count >= neotmux->bb.capacity) {                       \
      neotmux->bb.capacity *= 2;                                               \
      neotmux->bb.buffer = realloc(neotmux->bb.buffer, neotmux->bb.capacity);  \
    }                                                                          \
    memcpy(neotmux->bb.buffer + neotmux->bb.n, buf, count);                    \
    neotmux->bb.n += count;                                                    \
  } while (0)

#define bb_color(color)                                                        \
  do {                                                                         \
    char buf[32];                                                              \
    int n = snprintf(buf, 32, "\033[38;5;%dm", color);                         \
    bb_write(buf, n);                                                          \
  } while (0)

#endif
