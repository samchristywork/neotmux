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

int main() {
}
