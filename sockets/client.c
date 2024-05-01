#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

struct termios term;
enum Mode { MODE_NORMAL, MODE_CONTROL };
fd_set fds;

bool ctrl_c_pressed = false;

void ctrl_c_callback(int sig) {
  ctrl_c_pressed = true;

  signal(sig, ctrl_c_callback);
}

void raw_mode() {
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void reset_mode() {
  term.c_lflag |= ICANON | ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void configure_server(struct sockaddr_in *server) {
  server->sin_addr.s_addr = inet_addr("127.0.0.1");
  server->sin_family = AF_INET;
  server->sin_port = htons(8888);
}

int connect_to_server(int sock, struct sockaddr_in *server) {
  if (connect(sock, (struct sockaddr *)server, sizeof(*server)) < 0) {
    perror("connect failed. Error");
    return 1;
  }
  return 0;
}

void receive_message(int sock) {
  FD_ZERO(&fds);
  FD_SET(sock, &fds);

  char server_reply[2000];
  int retval = select(sock + 1, &fds, NULL, NULL, NULL);
  if (retval == -1) {
    perror("select()");
  } else if (retval) {
    int read_size = recv(sock, server_reply, 2000, 0);
    if (read_size == 0) {
      puts("Server disconnected");
      exit(EXIT_FAILURE);
    } else if (read_size == -1) {
      perror("recv failed");
      exit(EXIT_FAILURE);
    }
    write(STDOUT_FILENO, server_reply, read_size);
    fflush(stdout);
  } else {
    puts("Timeout");
  }
}

void handle_key(int numRead, char *buf, int sock, char *str, char c) {
  if (numRead == 1 && buf[1] == c) {
    write(sock, str, strlen(str));
  }
}

void event_loop(int sock) {
  raw_mode();

  int mode = MODE_NORMAL;
  char buf[32];
  buf[0] = 'e';
  while (1) {
    ssize_t numRead = read(STDIN_FILENO, buf + 1, 31);
    if (numRead <= 0) {
      exit(EXIT_SUCCESS);
    }

    if (mode == MODE_NORMAL) {
      if (numRead == 1 && buf[1] == 1) { // Ctrl-A
        mode = MODE_CONTROL;
      } else {
        write(sock, buf, numRead + 1);
      }
    } else if (mode == MODE_CONTROL) {
      handle_key(numRead, buf, sock, "cSplit", '|');
      handle_key(numRead, buf, sock, "cVSplit", '_');
      handle_key(numRead, buf, sock, "cList", 'i');
      handle_key(numRead, buf, sock, "cCreate", 'c');
      handle_key(numRead, buf, sock, "cReload", 'r');
      handle_key(numRead, buf, sock, "cLeft", 'h');
      handle_key(numRead, buf, sock, "cDown", 'j');
      handle_key(numRead, buf, sock, "cUp", 'k');
      handle_key(numRead, buf, sock, "cRight", 'l');
      handle_key(numRead, buf, sock, "cNext", 'n');
      handle_key(numRead, buf, sock, "cPrev", 'p');

      if (numRead == 1 && buf[1] == 'q') {
        break;
      }

      mode = MODE_NORMAL;
    }
  }

  reset_mode();
}

void *receive_messages(void *socket_desc) {
  int sock = *(int *)socket_desc;

  while (1) {
    receive_message(sock);
  }
  pthread_exit(NULL);
}

int client() {
  signal(SIGINT, ctrl_c_callback);

  struct sockaddr_in server;
  configure_server(&server);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("Could not create socket");
    close(sock);
    return EXIT_FAILURE;
  }

  if (connect_to_server(sock, &server)) {
    close(sock);
    return EXIT_FAILURE;
  }

  write(STDOUT_FILENO, "\033[?1049h", 8); // Alternate screen

  pthread_t thread;
  pthread_create(&thread, NULL, receive_messages, (void *)&sock);

  event_loop(sock);

  pthread_cancel(thread);
  pthread_join(thread, NULL);
  close(sock);

  write(STDOUT_FILENO, "\033[?1049l", 8); // Normal screen

  return EXIT_SUCCESS;
}
