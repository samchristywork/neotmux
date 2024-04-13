#define _XOPEN_SOURCE 600

#include <fcntl.h>
#include <server.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
              const struct winsize *ws, struct termios oldTermios) {
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

    if (tcsetattr(fd, TCSANOW, &oldTermios) == -1) {
      exit(EXIT_FAILURE);
    }

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
