#include <ctype.h>
#include <stdio.h>
#include <vterm.h>

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

void printLastWritten(char *lastWritten, size_t lastWrittenLen) {
  printf("len: %ld\r\n", lastWrittenLen);
  for (size_t i = 0; i < lastWrittenLen; i++) {
    if (isprint(lastWritten[i])) {
      printf("%c", lastWritten[i]);
    } else {
      printf("\\x%02x", lastWritten[i]);
    }
  }
}
