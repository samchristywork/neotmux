#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "server.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s [server|client]\n", argv[0]);
    return EXIT_FAILURE;
  }

  int port = 8888;

  if (strcmp(argv[1], "server") == 0) {
    return server(port);
  } else if (strcmp(argv[1], "client") == 0) {
    return client(port);
  } else {
    printf("Usage: %s [server|client]\n", argv[0]);
    return EXIT_FAILURE;
  }
}
