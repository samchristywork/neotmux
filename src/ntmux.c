#define _XOPEN_SOURCE 600

#include <fcntl.h>
#include <layout.h>
#include <render.h>
#include <server.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <vterm.h>

#define BUF_SIZE 256

struct termios oldTermios;

char lastWritten[BUF_SIZE];
size_t lastWrittenLen = 0;

static void ttyReset() {
  if (tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios) == -1) {
    exit(EXIT_FAILURE);
  }

  makeCursorVisible();
  normalScreen();
}

int main() {
  if (tcgetattr(STDIN_FILENO, &oldTermios) == -1) {
    exit(EXIT_FAILURE);
  }

  struct winsize ws;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0) {
    exit(EXIT_FAILURE);
  }

  Window windows[2];
  windows[0].process = -1;
  windows[0].width = ws.ws_col;
  windows[0].height = ws.ws_row / 2;
  windows[0].col = 0;
  windows[0].row = 0;
  windows[0].vt = NULL;
  windows[0].vts = NULL;

  windows[1].process = -1;
  windows[1].width = ws.ws_col;
  windows[1].height = ws.ws_row / 2 - 1;
  windows[1].col = 0;
  windows[1].row = ws.ws_row / 2 + 1;
  windows[1].vt = NULL;
  windows[1].vts = NULL;

  int nWindows = 2;

  for (int i = 0; i < nWindows; i++) {
    struct winsize ws;
    ws.ws_col = windows[i].width;
    ws.ws_row = windows[i].height;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    char childName[MAX_NAME];
    pid_t childPid =
        ptyFork(&windows[i].process, childName, MAX_NAME, &ws, oldTermios);
    if (childPid == -1) {
      exit(EXIT_FAILURE);
    }

    if (childPid == 0) { // Child
      char *shell = getenv("SHELL");
      execlp(shell, shell, (char *)NULL);
      exit(EXIT_FAILURE);
    }

    initScreen(&windows[i].vt, &windows[i].vts, windows[i].height,
               windows[i].width);
  }

  oldTermios = ttySetRaw();

  if (atexit(ttyReset) != 0) {
    exit(EXIT_FAILURE);
  }

  alternateScreen();
  makeCursorInvisible();

  int activeTerm = 0;

  while (true) {
    fd_set inFds;
    FD_ZERO(&inFds);
    FD_SET(fifo_fd, &inFds);
    for (int k = 0; k < nWindows; k++) {
      FD_SET(windows[k].process, &inFds);
    }

    if (select(fifo_fd + 1, &inFds, NULL, NULL, NULL) ==
        -1) {
      exit(EXIT_FAILURE);
    }

    char buf[BUF_SIZE];
    if (FD_ISSET(STDIN_FILENO, &inFds)) {
      ssize_t numRead = read(STDIN_FILENO, buf, BUF_SIZE);
      if (numRead <= 0) {
        exit(EXIT_SUCCESS);
      }

      lastWrittenLen = numRead;
      strncpy(lastWritten, buf, numRead);

      if (lastWrittenLen == 3 && lastWritten[0] == 27 && lastWritten[1] == 91 &&
          lastWritten[2] == 67) {
        activeTerm = 1;
      } else if (lastWrittenLen == 3 && lastWritten[0] == 27 &&
                 lastWritten[1] == 91 && lastWritten[2] == 68) {
        activeTerm = 0;
      } else {
        if (write(windows[activeTerm].process, buf, numRead) != numRead) {
          exit(EXIT_FAILURE);
        }
      }
    }

    for (int k = 0; k < nWindows; k++) {
      if (FD_ISSET(windows[k].process, &inFds)) {
        ssize_t numRead = read(windows[k].process, buf, BUF_SIZE);
        if (numRead <= 0) {
          exit(EXIT_SUCCESS);
        }

        vterm_input_write(windows[k].vt, buf, numRead);
      }
    }
  }
  // vterm_free(vt);
}
