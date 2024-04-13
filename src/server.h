#ifndef SERVER_H
#define SERVER_H

#include <sys/ioctl.h>
#include <termios.h>
#include <vterm.h>

#define MAX_NAME 1000

void initScreen(VTerm **vt, VTermScreen **vts, int h, int w);

pid_t ptyFork(int *parentFd, char *childName, size_t len,
              const struct winsize *ws, struct termios oldTermios);

#endif
