#define _XOPEN_SOURCE 600

#include <fcntl.h>
#include <layout.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

// TODO: Determine sizes
#define MAX_NAME 1024
#define BUF_SIZE 1024

int inFifo_s;
int outFifo_s;
int controlFifo_s;

int timeDiffMs(struct timeval *lastTime) {
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);

  return (currentTime.tv_sec - lastTime->tv_sec) * 1000 +
         (currentTime.tv_usec - lastTime->tv_usec) / 1000;
}

static void cleanup_server() {
  close(inFifo_s);
  close(outFifo_s);
  close(controlFifo_s);
  printf("Server stopped\n");
}

void initScreen(VTerm **vt, VTermScreen **vts, int h, int w) {
  *vt = vterm_new(h, w);
  if (!vt) {
    exit(EXIT_FAILURE);
  }

  *vts = vterm_obtain_screen(*vt);

  // TODO: Implement callbacks
  static VTermScreenCallbacks callbacks;
  callbacks.damage = NULL;
  callbacks.moverect = NULL;
  callbacks.movecursor = NULL;
  callbacks.settermprop = NULL;
  callbacks.bell = NULL;
  callbacks.resize = NULL;
  callbacks.sb_pushline = NULL;
  callbacks.sb_popline = NULL;

  vterm_screen_reset(*vts, 1);
  vterm_screen_enable_altscreen(*vts, 1);
  vterm_screen_set_callbacks(*vts, &callbacks, NULL);
}

int ptyOpen(char *childName, size_t len) {
  int fd = posix_openpt(O_RDWR | O_NOCTTY);
  if (fd == -1) {
    exit(EXIT_FAILURE);
  }

  if (grantpt(fd) == -1) {
    exit(EXIT_FAILURE);
  }

  if (unlockpt(fd) == -1) {
    exit(EXIT_FAILURE);
  }

  char *p = ptsname(fd);
  if (p == NULL) {
    exit(EXIT_FAILURE);
  }

  if (strlen(p) < len) {
    strncpy(childName, p, len);
  } else {
    exit(EXIT_FAILURE);
  }

  return fd;
}

pid_t ptyFork(int *parentFd, char *childName, size_t len,
              const struct winsize *ws) {
  char name[MAX_NAME];
  int fd = ptyOpen(name, MAX_NAME);
  if (fd == -1) {
    exit(EXIT_FAILURE);
  }

  if (childName != NULL) {
    if (strlen(name) < len) {
      strncpy(childName, name, len);

    } else {
      exit(EXIT_FAILURE);
    }
  }

  pid_t childPid = fork();
  if (childPid == -1) {
    exit(EXIT_FAILURE);
  }

  if (childPid != 0) { // Parent
    *parentFd = fd;
    return childPid;
  }

  if (setsid() == -1) {
    exit(EXIT_FAILURE);
  }

  close(fd);

  {
    int fd = open(name, O_RDWR);
    if (fd == -1) {
      exit(EXIT_FAILURE);
    }

    //if (tcsetattr(fd, TCSANOW, &oldTermios) == -1) {
    //  exit(EXIT_FAILURE);
    //}

    if (ioctl(fd, TIOCSWINSZ, ws) == -1) {
      exit(EXIT_FAILURE);
    }

    if (dup2(fd, STDIN_FILENO) != STDIN_FILENO) {
      exit(EXIT_FAILURE);
    }

    if (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO) {
      exit(EXIT_FAILURE);
    }

    if (dup2(fd, STDERR_FILENO) != STDERR_FILENO) {
      exit(EXIT_FAILURE);
    }

    if (fd > STDERR_FILENO) {
      close(fd);
    }
  }

  return 0;
}

void server() {
  struct winsize ws;
  ws.ws_col = 80;
  ws.ws_row = 24;

  Window windows[2];
  windows[0].process = -1;
  windows[0].width = ws.ws_col;
  windows[0].height = ws.ws_row / 2;
  windows[0].col = 0;
  windows[0].row = 0;
  windows[0].vt = NULL;
  windows[0].vts = NULL;
  windows[0].closed = false;

  windows[1].process = -1;
  windows[1].width = ws.ws_col;
  windows[1].height = ws.ws_row / 2 - 1;
  windows[1].col = 0;
  windows[1].row = ws.ws_row / 2 + 1;
  windows[1].vt = NULL;
  windows[1].vts = NULL;
  windows[1].closed = false;

  int nWindows = 2;

  for (int i = 0; i < nWindows; i++) {
    struct winsize ws;
    ws.ws_col = windows[i].width;
    ws.ws_row = windows[i].height;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    char childName[MAX_NAME];
    pid_t childPid =
        ptyFork(&windows[i].process, childName, MAX_NAME, &ws);
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

  if (atexit(cleanup_server) != 0) {
    exit(EXIT_FAILURE);
  }

  int activeTerm = 0;

  char *inFifoName = "/tmp/ntmux.in";
  char *outFifoName = "/tmp/ntmux.out";
  char *controlFifoName = "/tmp/ntmux.control";
  mkfifo(inFifoName, 0666);
  mkfifo(outFifoName, 0666);
  mkfifo(controlFifoName, 0666);

  printf("Fifos created\n");

  printf("Opening in fifo\n");
  inFifo_s = open(inFifoName, O_RDWR);
  if (inFifo_s == -1) {
    exit(EXIT_FAILURE);
  }

  printf("Opening out fifo\n");
  outFifo_s = open(outFifoName, O_RDWR);
  if (outFifo_s == -1) {
    exit(EXIT_FAILURE);
  }

  printf("Opening control fifo\n");
  controlFifo_s = open(controlFifoName, O_RDWR);
  if (controlFifo_s == -1) {
    exit(EXIT_FAILURE);
  }

  printf("%d %d %d\n", inFifo_s, outFifo_s, controlFifo_s);

  printf("Server started\n");

  struct timeval lastTime;
  gettimeofday(&lastTime, NULL);

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1000;

  bool dirty = true;

  while (true) {
    fd_set inFds;
    FD_ZERO(&inFds);

    // Monitor inFifo_s, controlFifo_s and all ptys
    FD_SET(inFifo_s, &inFds);
    FD_SET(controlFifo_s, &inFds);
    for (int k = 0; k < nWindows; k++) {
      FD_SET(windows[k].process, &inFds);
    }

    if (select(controlFifo_s + 1, &inFds, NULL, NULL, &tv) == -1) {
      exit(EXIT_FAILURE);
    }

    // Send input from fifos to ptys
    char buf[BUF_SIZE];
    if (FD_ISSET(inFifo_s, &inFds)) {
      printf("Reading from in fifo: <");
      ssize_t numRead = read(inFifo_s, buf, BUF_SIZE);
      if (numRead <= 0) {
        exit(EXIT_SUCCESS);
      }

      for (int i = 0; i < numRead; i++) {
        printf("%c", buf[i]);
      }

      printf(">\n");

      if (write(windows[activeTerm].process, buf, numRead) != numRead) {
        exit(EXIT_FAILURE);
      }

      dirty = true;
    }

    // Process control sequences
    if (FD_ISSET(controlFifo_s, &inFds)) {
      printf("Reading from control fifo: <");
      ssize_t numRead = read(controlFifo_s, buf, BUF_SIZE);
      if (numRead <= 0) {
        exit(EXIT_SUCCESS);
      }

      for (int i = 0; i < numRead; i++) {
        printf("%c", buf[i]);
      }

      printf(">\n");

      if (strncmp(buf, "size\n", 5) == 0) {
        printf("Size control sequence:\n");

        int newWidth = 0;
        int newHeight = 0;

        int i = 5;
        while (buf[i] != ' ') {
          newWidth = newWidth * 10 + (buf[i] - '0');
          i++;
        }

        i++;
        while (buf[i] != '\n') {
          newHeight = newHeight * 10 + (buf[i] - '0');
          i++;
        }

        //vterm_set_size(windows[activeTerm].vt, newHeight, newWidth);

        //windows[activeTerm].height = newHeight;
        //windows[activeTerm].width = newWidth;

        ws.ws_row = newHeight;
        ws.ws_col = newWidth;

        calculateLayout(windows, nWindows, ws.ws_row, ws.ws_col);

        dirty = true;

        printf("Size set to %d %d\n", newHeight, newWidth);
      } else if (numRead == 5 && strncmp(buf, "show\n", 5) == 0) {
        printf("Show control sequence\n");
        renderScreen(outFifo_s, windows, nWindows, activeTerm, ws.ws_row,
                     ws.ws_col);
        printf("Screen rendered\n");
      } else {
        printf("Unknown command\n");
      }
    }

    // Send output from ptys to vterm
    for (int k = 0; k < nWindows; k++) {
      if (windows[k].closed) {
        continue;
      }

      if (FD_ISSET(windows[k].process, &inFds)) {
        printf("Reading from pty %d\n", k);

        ssize_t numRead = read(windows[k].process, buf, BUF_SIZE);
        if (numRead <= 0) {
          printf("Pty %d closed\n", k);
          windows[k].closed = true;

          printf("Trying to find a new active term\n");
          for (int activeTerm = 0; activeTerm < nWindows; activeTerm++) {
            if (!windows[activeTerm].closed) {
              break;
            }
          }
          activeTerm++;
          printf("New active term: %d\n", activeTerm);
          if (activeTerm == nWindows) {
            renderScreen(outFifo_s, windows, nWindows, 0, ws.ws_row,
                         ws.ws_col);
            exit(EXIT_SUCCESS);
          }
        } else {
          vterm_input_write(windows[k].vt, buf, numRead);
        }

        dirty = true;
      }
    }

    if (dirty && timeDiffMs(&lastTime) > 1000 / 60) {
      printf("Rendering screen\n");
      renderScreen(outFifo_s, windows, nWindows, activeTerm, ws.ws_row,
                   ws.ws_col);
      dirty = false;
      gettimeofday(&lastTime, NULL);
    }

    // TODO: Performance
    usleep(1000);
  }
}
