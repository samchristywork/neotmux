#include <fcntl.h>
#include <log.h>
#include <signal.h>
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

void cleanup_client() {
  if (tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios) == -1) {
    exit(EXIT_FAILURE);
  }

  printf("\033[?25h"); // Show cursor
  normalScreen();
  close(inFifo_c);
  close(outFifo_c);
  close(controlFifo_c);
  printf("Client stopped\n");
}

void die(const char *msg) {
  cleanup_client();
  printf("%s\n", msg);
  exit(EXIT_FAILURE);
}

void renderClient(int outFifo_c, size_t size) {
  // TODO: Ensure that we read the number of bytes the server says we should
  static char buf[BUF_SIZE];
  ssize_t numRead = read(outFifo_c, buf, BUF_SIZE);
  if (numRead <= 0) {
    die("Read");
  }

  char msg[100];
  snprintf(msg, 100, "Read %ld bytes", numRead);
  logMessage(msg);

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
  int len = sprintf(buf, "size\n%d %d\n", ws.ws_col, ws.ws_row - 1);
  write(controlFifo_c, buf, len);
}

void sizeChangeCallback(int sig) {
  setSize();
  write(controlFifo_c, "show\n", 5);

  signal(SIGWINCH, sizeChangeCallback);
}

char readOneChar(int fd) {
  char c;
  if (read(fd, &c, 1) != 1) {
    exit(EXIT_FAILURE);
  }

  return c;
}

size_t readOneSizeT(int fd) {
  size_t s;
  if (read(fd, &s, sizeof(size_t)) != sizeof(size_t)) {
    exit(EXIT_FAILURE);
  }

  return s;
}

void client() {
  signal(SIGWINCH, sizeChangeCallback);

  setupFifos();

  ttySetRaw();

  write(STDOUT_FILENO, "\033[?1049h", 8); // Alternate screen
  write(STDOUT_FILENO, "\033[?25l", 6);   // Hide cursor

  setSize();
  write(controlFifo_c, "show\n", 5);

  int mode = MODE_NORMAL;

  logMessage("Client started");

  while (true) {
    fd_set inFds;
    FD_ZERO(&inFds);

    // Monitor stdin and outFifo_c
    FD_SET(STDIN_FILENO, &inFds);
    FD_SET(outFifo_c, &inFds);

    if (select(outFifo_c + 1, &inFds, NULL, NULL, NULL) == -1) {
      // die("Select");
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
      } else if (mode == MODE_CONTROL && buf[0] == 'h') {
        write(controlFifo_c, "left\n", 5);
        mode = MODE_NORMAL;
      } else if (mode == MODE_CONTROL && buf[0] == 'l') {
        write(controlFifo_c, "right\n", 6);
        mode = MODE_NORMAL;
      } else if (mode == MODE_CONTROL && buf[0] == 'c') {
        write(controlFifo_c, "create\n", 7);
        mode = MODE_NORMAL;
      } else if (mode == MODE_CONTROL && buf[0] == 'q') {
        die("Exiting");
      } else if (mode == MODE_CONTROL) {
        mode = MODE_NORMAL;
      } else {
        if (write(inFifo_c, buf, numRead) != numRead) {
          die("Fifo");
        }
      }
    }

    // Render output from outFifo_c
    if (FD_ISSET(outFifo_c, &inFds)) {
      char c = readOneChar(outFifo_c);
      switch (c) {
      case 'r': {
        size_t screenSize = readOneSizeT(outFifo_c);
        char msg[100];
        snprintf(msg, 100, "Render %ld bytes", screenSize);
        logMessage(msg);
        renderClient(outFifo_c, screenSize);
        break;
      }
      case 'e': {
        logMessage("Exit message received");
        die("E");
        break;
      }
      }
    }
  }
}
