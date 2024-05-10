#ifndef LOG_H
#define LOG_H

void write_log(char *module, int line, int socket, char *msg);

#define WRITE_LOG(socket, fmt, ...)                                            \
  {                                                                            \
    char *str;                                                                 \
    asprintf(&str, fmt, ##__VA_ARGS__);                                        \
    write_log(__FILE__, __LINE__, socket, str);                                \
    free(str);                                                                 \
  }

#endif
