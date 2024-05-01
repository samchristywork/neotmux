#ifndef SESSION_H
#define SESSION_H

#include <vterm.h>

typedef enum Layout {
  LAYOUT_DEFAULT = 0,
  LAYOUT_EVEN_HORIZONTAL,
  LAYOUT_EVEN_VERTICAL,
  LAYOUT_MAIN_HORIZONTAL,
  LAYOUT_MAIN_VERTICAL,
  LAYOUT_TILED,
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

typedef struct Neotmux {
  Session *sessions;
  int session_count;
  int current_session;
} Neotmux;

void print_sessions(Neotmux *neotmux);
Pane *add_pane(Window *window, int col, int row, int width, int height);
Window *add_window(Session *session, char *title);
Session *add_session(Neotmux *neotmux, char *title);

#endif
