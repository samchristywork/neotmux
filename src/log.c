#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void logMessage(char *msg) {
  static FILE *f = NULL;
  if (f == NULL) {
    f = fopen("/tmp/ntmux.log", "a");
    if (f == NULL) {
      exit(EXIT_FAILURE);
    }
  }

  char currentTime[100];
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  strftime(currentTime, sizeof(currentTime), "%c", tm);
  fprintf(f, "%s: %s\n", currentTime, msg);
}
