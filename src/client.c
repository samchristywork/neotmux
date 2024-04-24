#include <fcntl.h>
#include <log.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define BUF_SIZE 100000

int inFifo_c;
int outFifo_c;
int controlFifo_c;

struct termios oldTermios;

typedef enum { MODE_NORMAL, MODE_CONTROL } Mode;

void makeCursorInvisible() { printf("\033[?25l"); }

void makeCursorVisible() { printf("\033[?25h"); }

void alternateScreen() { printf("\033[?1049h"); }

void normalScreen() { printf("\033[?1049l"); }

void ttySetRaw() {
  struct termios t;
  if (tcgetattr(STDIN_FILENO, &t) == -1) {
    exit(EXIT_FAILURE);
  }

  oldTermios = t;

  t.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
  t.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK | ISTRIP |
                 IXON | PARMRK);
  t.c_oflag &= ~OPOST;
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) {
    exit(EXIT_FAILURE);
  }
}

static void cleanup_client() {
  if (tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios) == -1) {
    exit(EXIT_FAILURE);
  }

  makeCursorVisible();
  normalScreen();
  close(inFifo_c);
  close(outFifo_c);
  close(controlFifo_c);
  printf("Client stopped\n");
}

void renderClient(int outFifo_c) {

  static char buf[BUF_SIZE];
  ssize_t numRead = read(outFifo_c, buf, BUF_SIZE);
  if (numRead <= 0) {
    exit(EXIT_SUCCESS);
  }

  write(STDOUT_FILENO, buf, numRead);
}

void setupFifos() {
  char *inFifoName = "/tmp/ntmux.in";
  char *outFifoName = "/tmp/ntmux.out";
  char *controlFifoName = "/tmp/ntmux.control";

  printf("Opening in fifo\n");
  inFifo_c = open(inFifoName, O_RDWR);
  if (inFifo_c == -1) {
    exit(EXIT_FAILURE);
  }

  printf("Opening out fifo\n");
  outFifo_c = open(outFifoName, O_RDWR);
  if (outFifo_c == -1) {
    exit(EXIT_FAILURE);
  }

  printf("Opening control fifo\n");
  controlFifo_c = open(controlFifoName, O_RDWR);
  if (controlFifo_c == -1) {
    exit(EXIT_FAILURE);
  }

  printf("%d %d %d\n", inFifo_c, outFifo_c, controlFifo_c);
}

void setSize() {

  struct winsize ws;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0) {
    exit(EXIT_FAILURE);
  }

  char buf[BUF_SIZE];
  int len = sprintf(buf, "size\n%d %d\n", ws.ws_col, ws.ws_row-2);
  write(controlFifo_c, buf, len);
}

void client() {
  setupFifos();

  ttySetRaw();

  if (atexit(cleanup_client) != 0) {
    exit(EXIT_FAILURE);
  }

  write(STDOUT_FILENO, "\033[?1049h", 8); // Alternate screen
  write(STDOUT_FILENO, "\033[?25l", 6);   // Hide cursor

  setSize();

  write(controlFifo_c, "show\n", 5);

  int mode = MODE_NORMAL;

  while (true) {
    fd_set inFds;
    FD_ZERO(&inFds);

    // Monitor stdin and outFifo_c
    FD_SET(STDIN_FILENO, &inFds);
    FD_SET(outFifo_c, &inFds);

    if (select(outFifo_c + 1, &inFds, NULL, NULL, NULL) == -1) {
      exit(EXIT_FAILURE);
    }

    // Send input from stdin to inFifo_c
    char buf[BUF_SIZE];
    if (FD_ISSET(STDIN_FILENO, &inFds)) {
      ssize_t numRead = read(STDIN_FILENO, buf, BUF_SIZE);
      if (numRead <= 0) {
        exit(EXIT_SUCCESS);
      }

      if (numRead == 1 && buf[0] == 1) { // Ctrl-a
        mode = MODE_CONTROL;
      } else if (mode == MODE_CONTROL && buf[0] == 'c') {
        write(controlFifo_c, "create\n", 7);
      } else if (mode == MODE_CONTROL && buf[0] == 'q') {
        exit(EXIT_SUCCESS);
      } else if (mode == MODE_CONTROL) {
        mode = MODE_NORMAL;
      } else {
        if (write(inFifo_c, buf, numRead) != numRead) {
          exit(EXIT_FAILURE);
        }
      }
    }

    // Render output from outFifo_c
    if (FD_ISSET(outFifo_c, &inFds)) {
      renderClient(outFifo_c);
    }
  }
}
