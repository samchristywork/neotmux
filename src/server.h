#ifndef SERVER_H
#define SERVER_H

int start_server(int sock, char *name, char *log_filename, char **commands,
                 int nCommands);
void run_command(int socket, char *buf, int read_size);

#endif
