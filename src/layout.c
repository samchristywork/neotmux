#include <layout.h>
#include <sys/ioctl.h>

void updateLayout(Pane *panes, int nPanes) {
  for (int i = 0; i < nPanes; i++) {
    vterm_set_size(panes[i].vt, panes[i].height, panes[i].width);

    struct winsize ws;
    ws.ws_row = panes[i].height;
    ws.ws_col = panes[i].width;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    if (ioctl(panes[i].process, TIOCSWINSZ, &ws) < 0) {
      exit(EXIT_FAILURE);
    }
  }
}

void evenVerticalLayout(Pane *panes, int nPanes, int height, int width) {
  int remaining = height - nPanes + 1;
  int row = 0;

  for (int i = 0; i < nPanes; i++) {
    panes[i].row = row;
    panes[i].col = 0;
    panes[i].height = remaining / (nPanes - i);
    panes[i].width = width;

    row += panes[i].height + 1;
    remaining -= panes[i].height;
  }

  updateLayout(panes, nPanes);
}

void evenHorizontalLayout(Pane *panes, int nPanes, int height, int width) {
  int remaining = width - nPanes + 1;
  int col = 0;

  for (int i = 0; i < nPanes; i++) {
    panes[i].row = 0;
    panes[i].col = col;
    panes[i].height = height;
    panes[i].width = remaining / (nPanes - i);

    col += panes[i].width + 1;
    remaining -= panes[i].width;
  }

  updateLayout(panes, nPanes);
}

void mainVerticalLayout(Pane *panes, int nPanes, int height, int width) {
  if (nPanes <= 1) {
    evenHorizontalLayout(panes, nPanes, height, width);
    return;
  }

  panes[0].row = 0;
  panes[0].col = 0;
  panes[0].height = height;
  panes[0].width = width / 2;

  int remaining = height;
  for (int i = 1; i < nPanes; i++) {
    panes[i].row = height - remaining;
    panes[i].col = width / 2 + 1;
    panes[i].height = remaining / (nPanes - i);
    panes[i].width = width / 2;

    remaining -= panes[i].height;
    remaining--;
  }

  updateLayout(panes, nPanes);
}

void mainHorizontalLayout(Pane *panes, int nPanes, int height, int width) {
  if (nPanes <= 1) {
    evenVerticalLayout(panes, nPanes, height, width);
    return;
  }

  panes[0].row = 0;
  panes[0].col = 0;
  panes[0].height = height / 2;
  panes[0].width = width;

  int remaining = width;
  for (int i = 1; i < nPanes; i++) {
    panes[i].row = height / 2 + 1;
    panes[i].col = width - remaining;
    panes[i].height = height / 2;
    panes[i].width = remaining / (nPanes - i);

    remaining -= panes[i].width;
    remaining--;
  }

  updateLayout(panes, nPanes);
}

void calculateLayout(Pane *panes, int nPanes, int height, int width) {
  mainHorizontalLayout(panes, nPanes, height, width);
}
