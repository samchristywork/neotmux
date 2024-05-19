#include <stdio.h>
#include <time.h>

#include "log.h"
#include "session.h"

extern Neotmux *neotmux;
LOG_TYPE log_level = 0;

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

void write_log(LOG_TYPE type, char *module, int line, int socket, char *msg) {
  if (type < log_level) {
    return;
  }

  const char *yellow = "\033[33m";
  const char *grey = "\033[90m";
  const char *blue = "\033[34m";
  const char *reset = "\033[0m";

  module = normalize_module_name(module);

  char str[20];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(str, sizeof(str), "%H:%M:%S", t);

  const char *type_string;
  switch (type) {
  case LOG_PERF:
    type_string = "\033[90mPERF";
    break;
  case LOG_EVENT:
    type_string = "\033[36mEVENT";
    break;
  case LOG_INFO:
    type_string = "INFO";
    break;
  case LOG_WARN:
    type_string = "\033[33mWARN";
    break;
  case LOG_ERROR:
    type_string = "\033[31mERROR";
    break;
  }
  fprintf(neotmux->log, "%s %s%s %s%d %s%3d %s%s%s: %s\n", type_string, grey,
          str, blue, socket, grey, line, yellow, module, reset, msg);

  free(module);

  fflush(neotmux->log);
}
