#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"

Arg args[52];
int max_long_name_len = 0;

void add_arg(char short_name, const char *long_name, const char *description,
             FLAG flags) {
  int len = strlen(long_name);
  if (len > max_long_name_len) {
    max_long_name_len = len;
  }
  if (short_name >= 'a' && short_name <= 'z') {
    args[short_name - 'a'] = (Arg){long_name, description, flags};
  } else if (short_name >= 'A' && short_name <= 'Z') {
    args[short_name - 'A' + 26] = (Arg){long_name, description, flags};
  } else {
    fprintf(stderr, "Invalid short name: %c\n", short_name);
    exit(EXIT_FAILURE);
  }
}

void usage(const char *program_name) {
  fprintf(stderr, "Usage: %s [options]\n", program_name);
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  for (int i = 0; i < 52; i++) {
    if (args[i].long_name != NULL) {
      fprintf(stderr, "  ");
      fprintf(stderr, "-%c", i < 26 ? 'a' + i : 'A' + i - 26);
      fprintf(stderr, ", ");
      fprintf(stderr, "--%s:", args[i].long_name);
      for (int j = 0; j < max_long_name_len - strlen(args[i].long_name); j++) {
        fprintf(stderr, " ");
      }
      fprintf(stderr, "  %s\n", args[i].description);
    }
  }
  exit(EXIT_FAILURE);
}

Arg get_arg_by_short_name(char short_name) {
  if (short_name >= 'a' && short_name <= 'z') {
    return args[short_name - 'a'];
  } else if (short_name >= 'A' && short_name <= 'Z') {
    return args[short_name - 'A' + 26];
  } else {
    fprintf(stderr, "Invalid short name: %c\n", short_name);
    exit(EXIT_FAILURE);
  }
}

bool match_short_name(char short_name, char *arg) {
  if (strlen(arg) != 2) {
    return false;
  }

  return strlen(arg) == 2 && arg[0] == '-' && arg[1] == short_name;
}

bool match_long_name(const char *long_name, char *arg) {
  if (strlen(arg) <= 2) {
    return false;
  }

  return strlen(arg) > 2 && arg[0] == '-' && arg[1] == '-' &&
         strcmp(long_name, arg + 2) == 0;
}

int get_arg_int(int argc, char *argv[], char short_name, int default_value) {
  Arg arg = get_arg_by_short_name(short_name);
  for (int i = 1; i < argc; i++) {
    if (match_short_name(short_name, argv[i]) ||
        match_long_name(arg.long_name, argv[i])) {
      if (i + 1 < argc) {
        int ret = atoi(argv[i + 1]);
        if (ret == 0) {
          return default_value;
        } else {
          return ret;
        }
      }
    }
  }
  return default_value;
}

bool get_arg_bool(int argc, char *argv[], char short_name, bool default_value) {
  Arg arg = get_arg_by_short_name(short_name);
  for (int i = 1; i < argc; i++) {
    if (match_short_name(short_name, argv[i]) ||
        match_long_name(arg.long_name, argv[i])) {
      return true;
    }
  }
  return default_value;
}

char *get_arg_string(int argc, char *argv[], char short_name,
                     char *default_value) {
  Arg arg = get_arg_by_short_name(short_name);
  for (int i = 1; i < argc; i++) {
    if (match_short_name(short_name, argv[i]) ||
        match_long_name(arg.long_name, argv[i])) {
      if (i + 1 < argc) {
        return argv[i + 1];
      }
    }
  }
  return default_value;
}
