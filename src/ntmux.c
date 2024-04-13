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

void printColor(char *prefix, VTermColor *color) {
  printf("%s", prefix);
  if (VTERM_COLOR_IS_RGB(color)) {
    printf("  RGB:\r\n");
    printf("    Red: %d\r\n", color->rgb.red);
    printf("    Green: %d\r\n", color->rgb.green);
    printf("    Blue: %d\r\n", color->rgb.blue);
  } else if (VTERM_COLOR_IS_INDEXED(color)) {
    printf("  Indexed:\r\n");
    printf("    Idx: %d\r\n", color->indexed.idx);
  }
}

void printCellInfo(VTermScreenCell cell) {
  printf("Cell Info:\r\n");
  // printf("  Chars: %s\r\n", cell.chars);
  printf("  Width: %d\r\n", cell.width);
  printf("  Attrs:\r\n");
  printf("    Bold: %d\r\n", cell.attrs.bold);
  printf("    Underline: %d\r\n", cell.attrs.underline);
  printf("    Italic: %d\r\n", cell.attrs.italic);
  printf("    Blink: %d\r\n", cell.attrs.blink);
  printf("    Reverse: %d\r\n", cell.attrs.reverse);
  printf("    Strike: %d\r\n", cell.attrs.strike);
  printf("    Font: %d\r\n", cell.attrs.font);
  printf("    DWL: %d\r\n", cell.attrs.dwl);
  printf("    DHL: %d\r\n", cell.attrs.dhl);
  printColor("  FG:\r\n", &cell.fg);
  printColor("  BG:\r\n", &cell.bg);
}

void printLastWritten() {
  printf("len: %ld\r\n", lastWrittenLen);
  for (size_t i = 0; i < lastWrittenLen; i++) {
    if (isprint(lastWritten[i])) {
      printf("%c", lastWritten[i]);
    } else {
      printf("\\x%02x", lastWritten[i]);
    }
  }
}

bool isInRect(int row, int col, Rect *rect) {
  if (row < rect->row || row >= rect->row + rect->height) {
    return false;
  }

  if (col < rect->col || col >= rect->col + rect->width) {
    return false;
  }

  return true;
}

void renderCell(Window *window, int row, int col) {
  VTermPos pos;
  pos.row = row;
  pos.col = col;
  VTermScreen *vts = window->vts;

  VTermScreenCell cell;
  vterm_screen_get_cell(vts, pos, &cell);

  char c = cell.chars[0];
  if (cell.width == 1 && (isalnum(c) || c == ' ' || ispunct(c))) {
    printf("%c", c);
  } else if (c == 0) {
    printf(" ");
  } else {
    printf("?");
  }
}

int main() {
}
