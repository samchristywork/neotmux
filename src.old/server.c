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
int max_fifo = 0;

int timeDiffMs(struct timeval *lastTime) {
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);

  return (currentTime.tv_sec - lastTime->tv_sec) * 1000 +
         (currentTime.tv_usec - lastTime->tv_usec) / 1000;
}

static void cleanup_server() {
  write(outFifo_s, "e", 1);
  sync();
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

  vterm_set_utf8(*vt, 1);
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

    // if (tcsetattr(fd, TCSANOW, &oldTermios) == -1) {
    //   exit(EXIT_FAILURE);
    // }

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

void addPane(Pane **panes, int *nPanes) {
  *panes = realloc(*panes, (*nPanes + 1) * sizeof(Pane));
  if (*panes == NULL) {
    exit(EXIT_FAILURE);
  }

  (*panes)[*nPanes].process = -1;
  (*panes)[*nPanes].width = 80;
  (*panes)[*nPanes].height = 24;
  (*panes)[*nPanes].col = 0;
  (*panes)[*nPanes].row = 0;
  (*panes)[*nPanes].vt = NULL;
  (*panes)[*nPanes].vts = NULL;
  (*panes)[*nPanes].closed = false;

  (*nPanes)++;

  struct winsize ws;
  ws.ws_col = 80;
  ws.ws_row = 24;
  ws.ws_xpixel = 0;
  ws.ws_ypixel = 0;

  char childName[MAX_NAME];
  pid_t childPid =
      ptyFork(&(*panes)[*nPanes - 1].process, childName, MAX_NAME, &ws);
  if (childPid == -1) {
    exit(EXIT_FAILURE);
  }

  if (childPid == 0) { // Child
    char *shell = getenv("SHELL");
    execlp(shell, shell, (char *)NULL);
    exit(EXIT_FAILURE);
  }

  initScreen(&(*panes)[*nPanes - 1].vt, &(*panes)[*nPanes - 1].vts,
             (*panes)[*nPanes - 1].height, (*panes)[*nPanes - 1].width);

  if (max_fifo < (*panes)[*nPanes - 1].process) {
    max_fifo = (*panes)[*nPanes - 1].process;
  }
}

void server() {
  struct winsize ws;
  ws.ws_col = 80;
  ws.ws_row = 24;

  Pane *panes = malloc(1);
  int nPanes = 0;

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
  max_fifo = controlFifo_s;

  printf("%d %d %d\n", inFifo_s, outFifo_s, controlFifo_s);

  printf("Server started\n");

  struct timeval lastTime;
  gettimeofday(&lastTime, NULL);

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1000;

  bool dirty = true;

  addPane(&panes, &nPanes);
  addPane(&panes, &nPanes);
  addPane(&panes, &nPanes);

  calculateLayout(panes, nPanes, ws.ws_row, ws.ws_col);

  while (true) {
    fd_set inFds;
    FD_ZERO(&inFds);

    // Monitor inFifo_s, controlFifo_s and all ptys
    FD_SET(inFifo_s, &inFds);
    FD_SET(controlFifo_s, &inFds);
    for (int k = 0; k < nPanes; k++) {
      FD_SET(panes[k].process, &inFds);
    }

    if (select(max_fifo + 1, &inFds, NULL, NULL, &tv) == -1) {
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

      if (write(panes[activeTerm].process, buf, numRead) != numRead) {
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

        // vterm_set_size(panes[activeTerm].vt, newHeight, newWidth);

        // panes[activeTerm].height = newHeight;
        // panes[activeTerm].width = newWidth;

        ws.ws_row = newHeight;
        ws.ws_col = newWidth;

        calculateLayout(panes, nPanes, ws.ws_row, ws.ws_col);

        dirty = true;

        printf("Size set to %d %d\n", newHeight, newWidth);
      } else if (numRead == 7 && strncmp(buf, "create\n", 7) == 0) {
        printf("Create control sequence\n");
        addPane(&panes, &nPanes);
        calculateLayout(panes, nPanes, ws.ws_row, ws.ws_col);
        dirty = true;
      } else if (numRead == 6 && strncmp(buf, "right\n", 6) == 0) {
        activeTerm++;
        if (activeTerm == nPanes) {
          activeTerm = 0;
        }
        dirty = true;
      } else if (numRead == 5 && strncmp(buf, "left\n", 5) == 0) {
        activeTerm--;
        if (activeTerm == -1) {
          activeTerm = nPanes - 1;
        }
        dirty = true;
      } else if (numRead == 5 && strncmp(buf, "show\n", 5) == 0) {
        printf("Show control sequence\n");
        renderScreen(outFifo_s, panes, nPanes, activeTerm, ws.ws_row,
                     ws.ws_col);
        printf("Screen rendered\n");
      } else {
        printf("Unknown command\n");
      }
    }

    // Send output from ptys to vterm
    for (int k = 0; k < nPanes; k++) {
      if (panes[k].closed) {
        continue;
      }

      if (FD_ISSET(panes[k].process, &inFds)) {
        printf("Reading from pty %d\n", k);

        ssize_t numRead = read(panes[k].process, buf, BUF_SIZE);
        if (numRead <= 0) {
          printf("Pty %d closed\n", k);
          panes[k].closed = true;

          printf("Trying to find a new active term\n");
          for (int activeTerm = 0; activeTerm < nPanes; activeTerm++) {
            if (!panes[activeTerm].closed) {
              break;
            }
          }
          activeTerm++;
          printf("New active term: %d\n", activeTerm);
          if (activeTerm == nPanes) {
            write(outFifo_s, "e", 1);
            exit(EXIT_SUCCESS);
          }
        } else {
          printf("%zu bytes read\n", numRead);
          vterm_input_write(panes[k].vt, buf, numRead);
        }

        dirty = true;
      }
    }

    if (dirty && timeDiffMs(&lastTime) > 1000 / 60) {
      printf("Rendering screen\n");
      renderScreen(outFifo_s, panes, nPanes, activeTerm, ws.ws_row, ws.ws_col);
      dirty = false;
      gettimeofday(&lastTime, NULL);
    }

    // TODO: Performance
    usleep(1000);
  }
}
