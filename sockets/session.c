#include <stdio.h>
#include <stdlib.h>

#include "session.h"

void print_session(Session *session) {
  printf("Session: %s\n", session->title);
  printf("  Window Count: %d\n", session->window_count);
  printf("  Current Window: %d\n", session->current_window);
  for (int i = 0; i < session->window_count; i++) {
    Window *window = &session->windows[i];
    printf("  Window: %s\n", window->title);
    printf("    Pane Count: %d\n", window->pane_count);
    printf("    Current Pane: %d\n", window->current_pane);
    switch (window->layout) {
      case LAYOUT_DEFAULT:
        printf("    Layout: %s\n", "Default");
        break;
      case LAYOUT_EVEN_HORIZONTAL:
        printf("    Layout: %s\n", "Even Horizontal");
        break;
      case LAYOUT_EVEN_VERTICAL:
        printf("    Layout: %s\n", "Even Vertical");
        break;
      case LAYOUT_MAIN_HORIZONTAL:
        printf("    Layout: %s\n", "Main Horizontal");
        break;
      case LAYOUT_MAIN_VERTICAL:
        printf("    Layout: %s\n", "Main Vertical");
        break;
      case LAYOUT_TILED:
        printf("    Layout: %s\n", "Tiled");
        break;
      default:
        printf("    Layout: %s\n", "Unknown");
        break;
    }
    for (int j = 0; j < window->pane_count; j++) {
      Pane *pane = &window->panes[j];
      printf("    Pane: %p\n", pane);
      printf("      col: %d\n", pane->col);
      printf("      row: %d\n", pane->row);
      printf("      width: %d\n", pane->width);
      printf("      height: %d\n", pane->height);
      if (pane->process.pid != -1) {
        printf("      Process: %s\n", pane->process.name);
        printf("        pid: %d\n", pane->process.pid);
        printf("        fd: %d\n", pane->process.fd);
      } else {
        printf("      Process: %s\n", "None");
      }
    }
  }
}

void print_sessions(Session *sessions, int count) {
  for (int i = 0; i < count; i++) {
    print_session(&sessions[i]);
  }
}

Pane *add_pane(Window *window, int col, int row, int width, int height) {
  int n = sizeof(*window->panes) * (window->pane_count + 1);
  window->panes = realloc(window->panes, n);
  Pane *pane = &window->panes[window->pane_count];
  pane->col = col;
  pane->row = row;
  pane->width = width;
  pane->height = height;
  pane->process.pid = -1;
  pane->process.name = NULL;
  pane->process.fd = -1;
  pane->process.closed = false;
  window->pane_count++;
  return pane;
}

Window *add_window(Session *session, char *title) {
  int n = sizeof(*session->windows) * (session->window_count + 1);
  session->windows = realloc(session->windows, n);
  Window *window = &session->windows[session->window_count];
  window->title = title;
  window->pane_count = 0;
  window->current_pane = 0;
  window->panes = NULL;
  window->layout = LAYOUT_MAIN_VERTICAL;
  window->width = 80;
  window->height = 24;
  window->zoom = -1;
  session->window_count++;
  return window;
}
