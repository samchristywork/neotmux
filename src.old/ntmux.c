#include <client.h>
#include <server.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1024

void usage() {
  fprintf(stderr, "Usage: ntmux [server|client]\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    usage();
  }

  if (strcmp(argv[1], "server") == 0) {
    server();
  } else if (strcmp(argv[1], "client") == 0) {
    client();
  } else {
    usage();
  }
}
