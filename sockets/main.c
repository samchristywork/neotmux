#include <stdio.h>
#include <string.h>

#include "server.h"
#include "client.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s [server|client]\n", argv[0]);
    return 1;
  }
  if (strcmp(argv[1], "server") == 0) {
    return server();
  } else if (strcmp(argv[1], "client") == 0) {
    return client();
  } else {
    printf("Usage: %s [server|client]\n", argv[0]);
    return 1;
  }
}
