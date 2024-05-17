#include <stdio.h>
#include <time.h>

#include "session.h"

extern Neotmux *neotmux;

char *normalize_module_name(char *module) {
  // Strip leading path
  char *last_slash = strrchr(module, '/');
  if (last_slash) {
    module = last_slash + 1;
  }

  char *ret = strdup(module);

  // Strip trailing .c
  char *last_dot = strrchr(ret, '.');
  if (last_dot) {
    last_dot[0] = '\0';
  }

  return ret;
}

void write_log(char *type, char *module, int line, int socket, char *msg) {
  const char *yellow = "\033[33m";
  const char *grey = "\033[90m";
  const char *blue = "\033[34m";
  const char *reset = "\033[0m";

  module = normalize_module_name(module);

  char str[20];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(str, sizeof(str), "%H:%M:%S", t);
  fprintf(neotmux->log, "%s %s%s %s%d %s%3d %s%s%s: %s\n", type, grey, str,
          blue, socket, grey, line, yellow, module, reset, msg);

  free(module);

  fflush(neotmux->log);
}
