#ifndef SESSION_H
#define SESSION_H

#include <lua5.4/lua.h>
#include <string.h>
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

typedef enum BarPosition { BAR_NONE, BAR_TOP, BAR_BOTTOM } BarPosition;

typedef struct Cursor {
  bool visible;
  int shape;
  bool mouse_active;
} Cursor;

typedef struct Process {
  char *name;
  int pid;
  int fd;
  VTerm *vt;
  VTermScreen *vts;
  Cursor cursor;
  bool closed;
} Process;

typedef struct Pane {
  int row;
  int col;
  int width;
  int height;
  Process *process;
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

typedef struct Buffer {
  char *buffer;
  int n;
  int capacity;
} Buffer;

typedef struct Neotmux {
  Session *sessions;
  int session_count;
  int current_session;
  pthread_mutex_t mutex;
  lua_State *lua;
  Buffer bb;
  VTermScreenCell prevCell;
  BarPosition barPos;
  int statusBarIdx;
} Neotmux;

#define buf_write(buf, count)                                                  \
  do {                                                                         \
    if (neotmux->bb.n + count >= neotmux->bb.capacity) {                       \
      neotmux->bb.capacity *= 2;                                               \
      neotmux->bb.buffer = realloc(neotmux->bb.buffer, neotmux->bb.capacity);  \
    }                                                                          \
    memcpy(neotmux->bb.buffer + neotmux->bb.n, buf, count);                    \
    neotmux->bb.n += count;                                                    \
  } while (0)

#define buf_color(color)                                                       \
  do {                                                                         \
    char buf[32];                                                              \
    int n = snprintf(buf, 32, "\033[38;5;%dm", color);                         \
    buf_write(buf, n);                                                         \
  } while (0)

Session *get_current_session(Neotmux *neotmux);
Window *get_current_window(Neotmux *neotmux);
Pane *get_current_pane(Neotmux *neotmux);

#endif