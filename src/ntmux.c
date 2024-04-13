#define _XOPEN_SOURCE 600

#include <client.h>
#include <fcntl.h>
#include <server.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <vterm.h>

#define BUF_SIZE 256

struct termios oldTermios;

char lastWritten[BUF_SIZE];
size_t lastWrittenLen = 0;

typedef struct Window {
  int process;
  char *shell;
  VTerm *vt;
  VTermScreen *vts;
  Rect *rect;
} Window;

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
  windows[0].rect = malloc(sizeof(Rect));
  windows[0].rect->width = ws.ws_col;
  windows[0].rect->height = ws.ws_row / 2;
  windows[0].rect->col = 0;
  windows[0].rect->row = 0;
  windows[0].shell = "/usr/bin/fish";
  windows[0].vt = NULL;
  windows[0].vts = NULL;

  windows[1].process = -1;
  windows[1].rect = malloc(sizeof(Rect));
  windows[1].rect->width = ws.ws_col;
  windows[1].rect->height = ws.ws_row / 2 - 1;
  windows[1].rect->col = 0;
  windows[1].rect->row = ws.ws_row / 2 + 1;
  windows[1].shell = "/usr/bin/bash";
  windows[1].vt = NULL;
  windows[1].vts = NULL;

  for (int i = 0; i < 2; i++) {
    struct winsize ws;
    ws.ws_col = windows[i].rect->width;
    ws.ws_row = windows[i].rect->height;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    char childName[MAX_NAME];
    pid_t childPid =
        ptyFork(&windows[i].process, childName, MAX_NAME, &ws, oldTermios);
    if (childPid == -1) {
      exit(EXIT_FAILURE);
    }

    if (childPid == 0) { // Child
      execlp(windows[i].shell, windows[i].shell, (char *)NULL);
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < 2; i++) {
    initScreen(&windows[i].vt, &windows[i].vts, windows[i].rect->height,
               windows[i].rect->width);
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
    FD_SET(STDIN_FILENO, &inFds);
    FD_SET(windows[0].process, &inFds);
    FD_SET(windows[1].process, &inFds);

    if (select(windows[1].process + 1, &inFds, NULL, NULL, NULL) == -1) {
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

    if (FD_ISSET(windows[0].process, &inFds)) {
      ssize_t numRead = read(windows[0].process, buf, BUF_SIZE);
      if (numRead <= 0) {
        exit(EXIT_SUCCESS);
      }

      vterm_input_write(windows[0].vt, buf, numRead);
    }

    if (FD_ISSET(windows[1].process, &inFds)) {
      ssize_t numRead = read(windows[1].process, buf, BUF_SIZE);
      if (numRead <= 0) {
        exit(EXIT_SUCCESS);
      }

      vterm_input_write(windows[1].vt, buf, numRead);
    }

    printf("\033[H"); // Move cursor to top left

    VTermPos cursorPos;
    vterm_state_get_cursorpos(vterm_obtain_state(windows[activeTerm].vt),
                              &cursorPos);

    for (int row = 0; row < ws.ws_row - 1; row++) {
      // printf("\033[K"); // Clear line
      for (int col = 0; col < ws.ws_col; col++) {
        if (cursorPos.row == row - windows[activeTerm].rect->row &&
            cursorPos.col == col - windows[activeTerm].rect->col) {
          printf("\033[7m");
        }
        bool isRendered = false;
        for (int k = 0; k < 2; k++) {
          if (isInRect(row, col, windows[k].rect)) {
            VTermPos pos;
            pos.row = row - windows[k].rect->row;
            pos.col = col - windows[k].rect->col;
            VTermScreen *vts = windows[k].vts;

            VTermScreenCell cell;
            vterm_screen_get_cell(vts, pos, &cell);
            renderCell(cell);
            isRendered = true;
            break;
          }
        }
        if (!isRendered) {
          printf("?");
        }
        if (cursorPos.row == row - windows[activeTerm].rect->row &&
            cursorPos.col == col - windows[activeTerm].rect->col) {
          printf("\033[0m");
        }
      }
      printf("\r\n");
    }
  }

  exit(EXIT_SUCCESS);
}
