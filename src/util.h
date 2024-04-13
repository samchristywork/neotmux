#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <vterm.h>
#include <ctype.h>

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

#endif
