#include <arpa/inet.h>
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

#define LICENSE_STRING                                                         \
  "Copyright (C) 2024 Sam Christy.\n"                                          \
  "License GPLv3+: GNU GPL version 3 or later "                                \
  "<http://gnu.org/licenses/gpl.html>\n"                                       \
  "\n"                                                                         \
  "This is free software; you are free to change and redistribute it.\n"       \
  "There is NO WARRANTY, to the extent permitted by law."

int init_client(char *name) {
  mkdir("/tmp/ntmux-1000", 0777);

  struct sockaddr_un server;
  server.sun_family = AF_UNIX;
  snprintf(server.sun_path, sizeof(server.sun_path), "/tmp/ntmux-1000/%s.sock",
           name);
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("Could not create socket");
    close(sock);
    exit(EXIT_FAILURE);
  }

  printf("Connecting\n");
  while (1) {
    const useconds_t one_second = 1000000;
    usleep(one_second / 10);
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
      perror("connect failed");
    } else {
      printf("Connected\n");
      break;
    }
  }

  return sock;
}

int init_server(char *name) {
  mkdir("/tmp/ntmux-1000", 0777);

  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) {
    fprintf(stderr, "Could not create socket");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_un server;
  server.sun_family = AF_UNIX;
  snprintf(server.sun_path, sizeof(server.sun_path), "/tmp/ntmux-1000/%s.sock",
           name);

  if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    puts("bind failed");
    exit(EXIT_FAILURE);
  }

  return sock;
}

int main(int argc, char *argv[]) {
  add_arg('c', "client", "Run Ntmux in client mode", ARG_NONE);
  add_arg('i', "inet", "Use INET sockets", ARG_NONE);
  add_arg('n', "name", "Name of the socket (default \"local\")", ARG_REQUIRED);
  add_arg('p', "port", "Port number to use (default 5097)", ARG_REQUIRED);
  add_arg('s', "server", "Run Ntmux in server mode", ARG_NONE);
  add_arg('u', "unix", "Use UNIX sockets (default)", ARG_NONE);
  add_arg('h', "help", "Show this help message", ARG_NONE);
  add_arg('v', "version", "Show the version number and license info", ARG_NONE);
  add_arg('l', "log", "Log file to use (default \"ntmux.log\")", ARG_REQUIRED);

  bool help = get_arg_bool(argc, argv, 'h', false);
  bool version = get_arg_bool(argc, argv, 'v', false);
  bool client_mode = get_arg_bool(argc, argv, 'c', false);
  bool server_mode = get_arg_bool(argc, argv, 's', false);
  bool use_inet = get_arg_bool(argc, argv, 'i', false);
  bool use_unix = get_arg_bool(argc, argv, 'u', true);
  char *name = get_arg_string(argc, argv, 'n', "local");
  char *log_filename = get_arg_string(argc, argv, 'l', "ntmux.log");
  // int port = get_arg_int(argc, argv, 'p', 5097);

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
    return EXIT_FAILURE;
  }

  // TODO: Don't check for NTMUX if running only in server mode.
  if (getenv("NTMUX") != NULL) {
    fprintf(stderr, "Cannot run Ntmux inside Ntmux\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (setenv("NTMUX", "true", 1) != 0) {
    perror("setenv");
    return EXIT_FAILURE;
  }

  if (client_mode && server_mode) {
    fprintf(stderr, "Cannot run in both client and server mode\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  } else if (client_mode && !server_mode) {
    int sock = init_client(name);
    return start_client(sock);
  } else if (!client_mode && server_mode) {
    int sock = init_server(name);
    return start_server(sock, name, log_filename);
  } else if (!client_mode && !server_mode) {
    pid_t pid = fork();
    if (pid == 0) {
      // Child process
      int sock = init_server(name);
      close(0);
      close(1);
      close(2);
      return start_server(sock, name, log_filename);
    } else if (pid > 0) {
      // Parent process
      int sock = init_client(name);
      return start_client(sock);
    } else {
      perror("fork");
      return EXIT_FAILURE;
    }
  }

  usage(argv[0]);
  return EXIT_FAILURE;
}
