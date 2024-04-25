#include <layout.h>
#include <sys/ioctl.h>

void evenVerticalLayout(Window *windows, int nWindows, int height, int width) {
  int remaining = height - nWindows + 1;
  int row = 0;

  for (int i = 0; i < nWindows; i++) {
    windows[i].row = row;
    windows[i].col = 0;
    windows[i].height = remaining / (nWindows - i);
    windows[i].width = width - 1;

    vterm_set_size(windows[i].vt, windows[i].height, windows[i].width);

    struct winsize ws;
    ws.ws_row = windows[i].height;
    ws.ws_col = windows[i].width;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    if (ioctl(windows[i].process, TIOCSWINSZ, &ws) < 0) {
      exit(EXIT_FAILURE);
    }

    row += windows[i].height + 1;
    remaining -= windows[i].height;
  }
}

void evenHorizontalLayout(Window *windows, int nWindows, int height,
                          int width) {
  int remaining = width - nWindows + 1;
  int col = 0;

  for (int i = 0; i < nWindows; i++) {
    windows[i].row = 0;
    windows[i].col = col;
    windows[i].height = height - 1;
    windows[i].width = remaining / (nWindows - i);

    vterm_set_size(windows[i].vt, windows[i].height, windows[i].width);

    struct winsize ws;
    ws.ws_row = windows[i].height;
    ws.ws_col = windows[i].width;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    if (ioctl(windows[i].process, TIOCSWINSZ, &ws) < 0) {
      exit(EXIT_FAILURE);
    }

    col += windows[i].width + 1;
    remaining -= windows[i].width;
  }
}

void calculateLayout(Window *windows, int nWindows, int height, int width) {
  // evenVerticalLayout(windows, nWindows, height, width);
  evenHorizontalLayout(windows, nWindows, height, width);
}
