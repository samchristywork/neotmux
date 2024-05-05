#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

int accept_connection(int socket_desc, struct sockaddr_in client) {
  int c = sizeof(struct sockaddr_in);
  int socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
  if (socket < 0) {
    perror("accept failed");
    exit(EXIT_FAILURE);
  }
  return socket;
}

void wait_for_connection(int socket_desc) {
  listen(socket_desc, 3);
  puts("Waiting for incoming connections...");
}

int *init_connection(int socket_desc) {
  wait_for_connection(socket_desc);
  struct sockaddr_in client;
  int *socket = malloc(sizeof(*socket));
  *socket = accept_connection(socket_desc, client);
  return socket;
}
