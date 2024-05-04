#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>

void handle_command(int socket, char *buf, int read_size);

#endif
