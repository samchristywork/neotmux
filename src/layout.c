#include <layout.h>
#include <sys/ioctl.h>

void evenVerticalLayout(Pane *panes, int nPanes, int height, int width) {
  int remaining = height - nPanes + 1;
  int row = 0;

  for (int i = 0; i < nPanes; i++) {
    panes[i].row = row;
    panes[i].col = 0;
    panes[i].height = remaining / (nPanes - i);
    panes[i].width = width - 1;

    vterm_set_size(panes[i].vt, panes[i].height, panes[i].width);

    struct winsize ws;
    ws.ws_row = panes[i].height;
    ws.ws_col = panes[i].width;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    if (ioctl(panes[i].process, TIOCSWINSZ, &ws) < 0) {
      exit(EXIT_FAILURE);
    }

    row += panes[i].height + 1;
    remaining -= panes[i].height;
  }
}

void evenHorizontalLayout(Pane *panes, int nPanes, int height, int width) {
  int remaining = width - nPanes + 1;
  int col = 0;

  for (int i = 0; i < nPanes; i++) {
    panes[i].row = 0;
    panes[i].col = col;
    panes[i].height = height;
    panes[i].width = remaining / (nPanes - i);

    vterm_set_size(panes[i].vt, panes[i].height, panes[i].width);

    struct winsize ws;
    ws.ws_row = panes[i].height;
    ws.ws_col = panes[i].width;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    if (ioctl(panes[i].process, TIOCSWINSZ, &ws) < 0) {
      exit(EXIT_FAILURE);
    }

    col += panes[i].width + 1;
    remaining -= panes[i].width;
  }
}

void calculateLayout(Pane *panes, int nPanes, int height, int width) {
  // evenVerticalLayout(panes, nPanes, height, width);
  evenHorizontalLayout(panes, nPanes, height, width);
}
