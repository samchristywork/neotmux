#include <stdio.h>
#include <time.h>

#include "session.h"

extern Neotmux *neotmux;

void write_log(char *module, int line, int socket, char *msg) {
  const char *yellow = "\033[33m";
  const char *grey = "\033[90m";
  const char *blue = "\033[34m";
  const char *reset = "\033[0m";

  char str[20];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(str, sizeof(str), "%H:%M:%S", t);
  fprintf(neotmux->log, "INFO %s%s %s%d %s%3d %s%s%s: %s\n", grey, str, blue,
          socket, grey, line, yellow, module, reset, msg);
}
