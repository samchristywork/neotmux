#define _XOPEN_SOURCE 600

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <vterm.h>

#define MAX_NAME 1000
#define BUF_SIZE 256

struct termios oldTermios;

char lastWritten[BUF_SIZE];
size_t lastWrittenLen = 0;

typedef struct Rect {
  int width;
  int height;
  int col;
  int row;
} Rect;

typedef struct Window {
  int process;
  char *shell;
  VTerm *vt;
  VTermScreen *vts;
  Rect *rect;
} Window;

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

void initScreen(VTerm **vt, VTermScreen **vts, int h, int w) {
  *vt = vterm_new(h, w);
  if (!vt) {
    exit(EXIT_FAILURE);
  }

  *vts = vterm_obtain_screen(*vt);

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

int main() {
}
