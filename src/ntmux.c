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

static void ttyReset() {
  if (tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios) == -1) {
    exit(EXIT_FAILURE);
  }

  makeCursorVisible();
  normalScreen();
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

  bool needsReset = false;

  if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
    printf("\033[48;5;%dm", cell.bg.indexed.idx);
    needsReset = true;
  } else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
    printf("\033[48;2;%d;%d;%dm", cell.bg.rgb.red, cell.bg.rgb.green,
           cell.bg.rgb.blue);
    needsReset = true;
  }

  if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
    printf("\033[38;5;%dm", cell.fg.indexed.idx);
    needsReset = true;
  } else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
    printf("\033[38;2;%d;%d;%dm", cell.fg.rgb.red, cell.fg.rgb.green,
           cell.fg.rgb.blue);
    needsReset = true;
  }

  char c = cell.chars[0];
  if (cell.width == 1 && (isalnum(c) || c == ' ' || ispunct(c))) {
    printf("%c", c);
  } else if (c == 0) {
    printf(" ");
  } else {
    printf("?");
  }

  if (needsReset) {
    printf("\033[0m");
  }
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
  windows[1].rect->height = ws.ws_row / 2-1;
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
    pid_t childPid = ptyFork(&windows[i].process, childName, MAX_NAME, &ws);
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

  ttySetRaw();

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
    vterm_state_get_cursorpos(vterm_obtain_state(windows[activeTerm].vt), &cursorPos);

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
            renderCell(&windows[k], row - windows[k].rect->row,
                       col - windows[k].rect->col);
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
