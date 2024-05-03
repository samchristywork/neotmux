#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "server.h"

void usage(const char *progname) {
  fprintf(stderr, "Usage: %s server|client [port]\n", progname);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  int port = 8888;

  if (argc > 2) {
    port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
      fprintf(stderr, "Invalid port number: %s\n", argv[2]);
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (strcmp(argv[1], "server") == 0) {
    return server(port);
  } else if (strcmp(argv[1], "client") == 0) {
    return client(port);
  } else {
    usage(argv[0]);
    return EXIT_FAILURE;
  }
}
