#ifndef ARGS_H
#define ARGS_H

typedef enum FLAG {
  ARG_NONE = 0,
  ARG_REQUIRED = 1,
} FLAG;

typedef struct Arg {
  const char *long_name;
  const char *description;
  FLAG flags;
} Arg;

void add_arg(char short_name, const char *long_name, const char *description,
             FLAG flags);
bool get_arg_bool(int argc, char *argv[], char short_name, bool default_value);
int get_arg_int(int argc, char *argv[], char short_name, int default_value);
char *get_arg_string(int argc, char *argv[], char short_name,
                     char *default_value);
void usage(const char *program_name);

#endif
