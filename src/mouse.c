#include <unistd.h>

#include "server.h"
#include "session.h"

extern Neotmux *neotmux;

typedef enum MouseEventType {
  MOUSE_UNKNOWN = 0,
  MOUSE_LEFT_CLICK = 32,
  MOUSE_MIDDLE_CLICK = 33,
  MOUSE_RIGHT_CLICK = 34,
  MOUSE_RELEASE = 35,
  MOUSE_LEFT_DRAG = 64,
  MOUSE_MIDDLE_DRAG = 65,
  MOUSE_MOVE = 67,
  MOUSE_RIGHT_DRAG = 66,
  MOUSE_WHEEL_DOWN = 96,
  MOUSE_WHEEL_UP = 97,
} MouseEventType;

typedef struct MouseEvent {
  MouseEventType type;
  int x;
  int y;
} MouseEvent;

bool deserialize_mouse_event(char *buf, int read_size, MouseEvent *event) {
  if (read_size < 6) {
    return false;
  } else if (buf[0] != 0x1b || buf[1] != '[' || buf[2] != 'M') {
    return false;
  }

  event->type = (unsigned char)buf[3];
  event->x = (unsigned char)buf[4] - 0x20;
  event->y = (unsigned char)buf[5] - 0x20;

  return true;
}

bool serialize_mouse_event(char *buf, int buf_size, MouseEvent *event) {
  if (buf_size < 6) {
    return false;
  }

  buf[0] = 0x1b;
  buf[1] = '[';
  buf[2] = 'M';
  buf[3] = event->type;
  buf[4] = event->x + 0x20;
  buf[5] = event->y + 0x20;

  return true;
}

bool is_inside_rect(VTermRect rect, VTermPos pos) {
  return pos.col >= rect.start_col && pos.col < rect.end_col &&
         pos.row >= rect.start_row && pos.row < rect.end_row;
}

bool handle_mouse(int socket, char *buf, int read_size) {
  if (read_size < 6) {
    return false;
  }

  MouseEvent event;
  if (!deserialize_mouse_event(buf + 1, read_size - 1, &event)) {
    return false;
  }

  Window *w = get_current_window(neotmux);
  if (w->zoom == -1) {
    if (event.type == MOUSE_LEFT_CLICK) {
      for (int i = 0; i < w->pane_count; i++) {
        if (w->current_pane == i) {
          continue;
        }

        Pane *p = &w->panes[i];
        VTermRect rect = {.start_col = p->col,
                          .start_row = p->row,
                          .end_col = p->col + p->width,
                          .end_row = p->row + p->height};
        VTermPos pos = {.col = event.x - 1, .row = event.y - 1};
        if (is_inside_rect(rect, pos)) {
          w->current_pane = i;
          return true;
        }
      }
    }
  }

  Pane *pane = get_current_pane(neotmux);
  if (pane->process->cursor.mouse_active == VTERM_PROP_MOUSE_NONE) {
    event.x -= pane->col;
    event.y -= pane->row;

    char buf[6];
    if (serialize_mouse_event(buf, 6, &event)) {
      if (event.type == MOUSE_LEFT_CLICK) {
        pane->selection.active = true;
        pane->selection.start.col = event.x;
        pane->selection.start.row = event.y;
        pane->selection.end.col = event.x;
        pane->selection.end.row = event.y;
        run_command(socket, "cReRender", 10);
        run_command(socket, "cRenderScreen", 14);
        run_command(socket, "cRenderBar", 11);
      } else if (event.type == MOUSE_LEFT_DRAG) {
        pane->selection.end.col = event.x;
        pane->selection.end.row = event.y;
        run_command(socket, "cReRender", 10);
        run_command(socket, "cRenderScreen", 14);
        run_command(socket, "cRenderBar", 11);
      } else if (event.type == MOUSE_RELEASE) {
        if (pane->selection.start.col == event.x &&
            pane->selection.start.row == event.y) {
          pane->selection.active = false;
          run_command(socket, "cReRender", 10);
          run_command(socket, "cRenderScreen", 14);
          run_command(socket, "cRenderBar", 11);
        }
        // pane->selection.active = false;
        // run_command(socket, "cReRender", 10);
        // run_command(socket, "cRenderScreen", 14);
      }
    }
  } else {
    event.x -= pane->col;
    event.y -= pane->row;

    char buf[6];
    if (serialize_mouse_event(buf, 6, &event)) {
      if (event.type == MOUSE_LEFT_CLICK) {
        write(pane->process->fd, buf, 6);
        return true;
      } else if (event.type == MOUSE_LEFT_DRAG) {
        write(pane->process->fd, buf, 6);
        return true;
      } else if (event.type == MOUSE_RELEASE) {
        write(pane->process->fd, buf, 6);
        return true;
      }
      // TODO: Implement other mouse events
    }
  }

  return false;
}
