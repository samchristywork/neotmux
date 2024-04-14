#include <layout.h>
#include <stdio.h>
#include <sys/ioctl.h>

void calculateLayout(Window *windows, int nWindows, int height, int width) {
  int row = 0;
  for (int i = 0; i < nWindows; i++) {
    windows[i].row = row;
    windows[i].col = 0;
    windows[i].height = height / nWindows-1;
    windows[i].width = width;

    row += windows[i].height+1;

    printf("Setting size of window %d to %d %d\n", i, windows[i].height,
           windows[i].width);
    vterm_set_size(windows[i].vt, windows[i].height, windows[i].width);

    struct winsize ws;
    ws.ws_row = windows[i].height;
    ws.ws_col = windows[i].width;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    if (ioctl(windows[i].process, TIOCSWINSZ, &ws) < 0) {
      exit(EXIT_FAILURE);
    }
  }
}
