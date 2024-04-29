#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

struct termios term;

int client_create_socket() {
  int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Could not create socket");
    return -1;
  }
  return socket_desc;
}

void rawMode() {
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void resetMode() {
  term.c_lflag |= ICANON | ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void configure_server(struct sockaddr_in *server) {
  server->sin_addr.s_addr = inet_addr("127.0.0.1");
  server->sin_family = AF_INET;
  server->sin_port = htons(8889);
}

int connect_to_server(int sock, struct sockaddr_in *server) {
  if (connect(sock, (struct sockaddr *)server, sizeof(*server)) < 0) {
    perror("connect failed. Error");
    return 1;
  }

  puts("Connected\n");
  return 0;
}

void receive_message_from_server(int sock, fd_set *fds, char *server_reply) {
  FD_ZERO(fds);
  FD_SET(sock, fds);

  int retval = select(sock + 1, fds, NULL, NULL, NULL);
  if (retval == -1) {
    perror("select()");
  } else if (retval) {
    int read_size = recv(sock, server_reply, 2000, 0);
    if (read_size == 0) {
      puts("Server disconnected");
      fflush(stdout);
    } else if (read_size == -1) {
      perror("recv failed");
    }
    puts(server_reply);
  } else {
    puts("Timeout");
  }
}

void send_message_to_server(int sock) {
  rawMode();

  while (1) {
    int c = getchar();
    if (c == 3) { // Ctrl-C
      break;
    } else if (c == 'q') {
      break;
    }
    printf("Sending: %c\n", c);
    write(sock, &c, 1);
  }

  resetMode();
}

int client() {
  char message[2000], server_reply[2000];

  int sock = client_create_socket();

  struct sockaddr_in server;
  configure_server(&server);

  if (connect_to_server(sock, &server)) {
    return 1;
  }

  fd_set fds;

  pid_t pid = fork();
  if (pid == 0) { // Child
    while (1) {
      receive_message_from_server(sock, &fds, server_reply);
    }
  } else {
    send_message_to_server(sock);
    kill(pid, SIGKILL);
  }

  close(sock);
  return 0;
}
