#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

int server_create_socket() {
  int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Could not create socket");
    return -1;
  }
  return socket_desc;
}

struct sockaddr_in setup_server(int socket_desc) {
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(8889);

  if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
    puts("bind failed");
    exit(1);
  }

  return server;
}

void wait_for_connections(int socket_desc) {
  listen(socket_desc, 3);
  puts("Waiting for incoming connections...");
}

int accept_connection(int socket_desc, struct sockaddr_in client) {
  int c = sizeof(struct sockaddr_in);
  int new_socket =
      accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
  if (new_socket < 0) {
    perror("accept failed");
    return -1;
  }
  return new_socket;
}

int server() {
  int socket_desc = server_create_socket();
  struct sockaddr_in server = setup_server(socket_desc);
  wait_for_connections(socket_desc);
  struct sockaddr_in client;
  int new_socket = accept_connection(socket_desc, client);

  while (1) {
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(new_socket, &fds);

    int retval = select(new_socket + 1, &fds, NULL, NULL, &tv);

    if (retval == -1) {
      perror("select()");
      break;
    } else if (retval) {
      char c;
      int read_size = recv(new_socket, &c, 1, 0);
      if (read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
        break;
      } else if (read_size == -1) {
        perror("recv failed");
        break;
      }
      printf("Received: %c\n", c);
    } else {
      puts("Timeout");
      write(new_socket, "Timeout", 7);
    }
  }

  return 0;
}
