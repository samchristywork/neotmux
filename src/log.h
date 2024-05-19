#ifndef LOG_H
#define LOG_H

typedef enum LOG_TYPE {
  LOG_PERF = 0,
  LOG_EVENT = 1,
  LOG_INFO = 2,
  LOG_WARN = 3,
  LOG_ERROR = 4
} LOG_TYPE;

void write_log(LOG_TYPE type, char *module, int line, int socket, char *msg);

#define WRITE_LOG(type, socket, fmt, ...)                                      \
  {                                                                            \
    char *str;                                                                 \
    asprintf(&str, fmt, ##__VA_ARGS__);                                        \
    write_log(type, __FILE__, __LINE__, socket, str);                          \
    free(str);                                                                 \
  }

#endif
