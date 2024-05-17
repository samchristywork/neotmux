#ifndef LOG_H
#define LOG_H

void write_log(char *type, char *module, int line, int socket, char *msg);

// TODO: Change to LOG_INFO
#define WRITE_LOG(socket, fmt, ...)                                            \
  {                                                                            \
    char *str;                                                                 \
    asprintf(&str, fmt, ##__VA_ARGS__);                                        \
    write_log("INFO", __FILE__, __LINE__, socket, str);                        \
    free(str);                                                                 \
  }

#endif
