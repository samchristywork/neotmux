#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "session.h"

extern Session *sessions;
extern int num_sessions;

enum BarPosition { TOP, BOTTOM };

int barPos = BOTTOM;
int bold = 0;
VTermColor bg = {0};
VTermColor fg = {0};

// TODO: Determine the correct buffer size
#define BUF_SIZE 100000

typedef struct BackBuffer {
  char buffer[BUF_SIZE];
  int n;
} BackBuffer;
BackBuffer bb;

unsigned int utf8_seqlen(long codepoint) {
  if (codepoint < 0x0000080)
    return 1;
  if (codepoint < 0x0000800)
    return 2;
  if (codepoint < 0x0010000)
    return 3;
  if (codepoint < 0x0200000)
    return 4;
  if (codepoint < 0x4000000)
    return 5;
  return 6;
}

int fill_utf8(long codepoint, char *str) {
  int nbytes = utf8_seqlen(codepoint);

  int b = nbytes;
  while (b > 1) {
    b--;
    str[b] = 0x80 | (codepoint & 0x3f);
    codepoint >>= 6;
  }

  switch (nbytes) {
  case 1:
    str[0] = (codepoint & 0x7f);
    break;
  case 2:
    str[0] = 0xc0 | (codepoint & 0x1f);
    break;
  case 3:
    str[0] = 0xe0 | (codepoint & 0x0f);
    break;
  case 4:
    str[0] = 0xf0 | (codepoint & 0x07);
    break;
  case 5:
    str[0] = 0xf8 | (codepoint & 0x03);
    break;
  case 6:
    str[0] = 0xfc | (codepoint & 0x01);
    break;
  }

  return nbytes;
}

void bb_write(const void *buf, size_t count) {
  memcpy(bb.buffer + bb.n, buf, count);
  bb.n += count;
}

bool is_in_rect(int row, int col, int rowRect, int colRect, int height,
                int width) {
  return row >= rowRect && row < rowRect + height && col >= colRect &&
         col < colRect + width;
}

bool compare_colors(VTermColor a, VTermColor b) {
  if (a.type != b.type) {
    return false;
  }

  if (a.type == VTERM_COLOR_DEFAULT_FG || a.type == VTERM_COLOR_DEFAULT_BG) {
    return true;
  }

  if (VTERM_COLOR_IS_INDEXED(&a) && VTERM_COLOR_IS_INDEXED(&b)) {
    return a.indexed.idx == b.indexed.idx;
  }

  if (VTERM_COLOR_IS_RGB(&a) && VTERM_COLOR_IS_RGB(&b)) {
    return a.rgb.red == b.rgb.red && a.rgb.green == b.rgb.green &&
           a.rgb.blue == b.rgb.blue;
  }

  return false;
}

void bb_write_indexed(int idx, int type) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;5;%dm", type, idx);
  bb_write(buf, n);
}

void bb_write_rgb(int red, int green, int blue, int type) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;2;%d;%d;%dm", type, red, green, blue);
  bb_write(buf, n);
}

void render_cell(VTermScreenCell cell) {
  if (cell.attrs.bold != bold) {
    bb_write("\033[1m", 4);
    bold = cell.attrs.bold;
  }

  if (!compare_colors(cell.bg, bg)) {
    if (cell.bg.type == VTERM_COLOR_DEFAULT_BG) {
    } else if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
      bb_write_indexed(cell.bg.indexed.idx, 48);
    } else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
      bb_write_rgb(cell.bg.rgb.red, cell.bg.rgb.green, cell.bg.rgb.blue, 48);
    }
    bg = cell.bg;
  }

  if (!compare_colors(cell.fg, fg)) {
    if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
      bb_write_indexed(cell.fg.indexed.idx, 38);
    } else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
      bb_write_rgb(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue, 38);
    }
    fg = cell.fg;
  }

  if (cell.chars[0] == 0) {
    bb_write(" ", 1);
  } else {
    for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i]; i++) {
      char bytes[6];
      int len = fill_utf8(cell.chars[i], bytes);
      bytes[len] = 0;
      bb_write(bytes, len);
    }
  }
}
