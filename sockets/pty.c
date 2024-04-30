#define _XOPEN_SOURCE 600

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

int pty_open(char *childName, size_t len) {
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

pid_t pty_fork(int *parentFd, char *childName, size_t len,
               const struct winsize *ws) {
  char name[len];
  int fd = pty_open(name, len);
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
