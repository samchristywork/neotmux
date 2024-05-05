#include <stdio.h>
#include <unistd.h>

#include "session.h"

extern Neotmux *neotmux;

bool is_inside_rect(VTermRect rect, VTermPos pos) {
  return pos.col >= rect.start_col && pos.col < rect.end_col &&
         pos.row >= rect.start_row && pos.row < rect.end_row;
}

bool handle_mouse(int socket, char *buf, int read_size) {
  bool dirty = false;
  printf("Mouse event (%d): ", socket);

  // Must be unsigned
  int b = (unsigned char)buf[4] & 0x03;
  int x = (unsigned char)buf[5] - 0x20;
  int y = (unsigned char)buf[6] - 0x20;

  printf("%d, %d, %d\n", x, y, b);
  fflush(stdout);

  if (b == 0) {
    printf("Left click\n");
    Session *session = &neotmux->sessions[neotmux->current_session];
    Window *w = &session->windows[session->current_window];
    for (int i = 0; i < w->pane_count; i++) {
      printf("Checking pane %d\n", i);
      printf("  %d, %d, %d, %d\n", w->panes[i].col, w->panes[i].row,
             w->panes[i].width, w->panes[i].height);
      Pane *p = &w->panes[i];
      VTermRect rect = {.start_col = p->col,
                        .start_row = p->row,
                        .end_col = p->col + p->width,
                        .end_row = p->row + p->height};
      VTermPos pos = {.col = x - 1, .row = y - 1};
      if (is_inside_rect(rect, pos)) {
        printf("Found pane %d\n", i);
        w->current_pane = i;
        dirty = true;
        break;
      }
    }
  }

  Session *session = &neotmux->sessions[neotmux->current_session];
  Window *window = &session->windows[session->current_window];
  Pane *pane = &window->panes[window->current_pane];

  // VTERM_PROP_MOUSE_NONE = 0,
  // VTERM_PROP_MOUSE_CLICK,
  // VTERM_PROP_MOUSE_DRAG,
  // VTERM_PROP_MOUSE_MOVE,
  // TODO: Handle all mouse states
  if (pane->process->cursor.mouse_active != VTERM_PROP_MOUSE_NONE) {
    write(pane->process->fd, buf + 1, read_size - 1);
    // TODO: This triggers a redraw, which we might not want.
  }

  return dirty;
}
