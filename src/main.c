#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "args.h"
#include "client.h"
#include "server.h"

#define VERSION_STRING "neotmux-1.0.0"

#define LICENSE_STRING "Copyright (C) 2024 Sam Christy.\n" \
"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n" \
"\n" \
"This is free software; you are free to change and redistribute it.\n" \
"There is NO WARRANTY, to the extent permitted by law."

int main(int argc, char *argv[]) {
  add_arg('c', "client", "Run Ntmux in client mode", ARG_NONE);
  add_arg('i', "inet", "Use INET sockets", ARG_NONE);
  add_arg('n', "name", "Name of the socket (default \"local\")", ARG_REQUIRED);
  add_arg('p', "port", "Port number to use (default 5097)", ARG_REQUIRED);
  add_arg('s', "server", "Run Ntmux in server mode", ARG_NONE);
  add_arg('u', "unix", "Use UNIX sockets (default)", ARG_NONE);
  add_arg('h', "help", "Show this help message", ARG_NONE);
  add_arg('v', "version", "Show the version number and license info", ARG_NONE);

  bool help = get_arg_bool(argc, argv, 'h', false);
  bool version = get_arg_bool(argc, argv, 'v', false);
  bool client_mode = get_arg_bool(argc, argv, 'c', false);
  bool server_mode = get_arg_bool(argc, argv, 's', false);
  bool use_inet = get_arg_bool(argc, argv, 'i', false);
  bool use_unix = get_arg_bool(argc, argv, 'u', true);
  char *name = get_arg_string(argc, argv, 'n', "local");
  int port = get_arg_int(argc, argv, 'p', 5097);

  if (version) {
    printf("%s\n\n%s\n", VERSION_STRING, LICENSE_STRING);
    return EXIT_SUCCESS;
  }

  if (help) {
    usage(argv[0]);
    return EXIT_SUCCESS;
  }

  if (use_inet && use_unix) {
    fprintf(stderr, "Cannot use both INET and UNIX sockets\n\n");
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  usage(argv[0]);
}
