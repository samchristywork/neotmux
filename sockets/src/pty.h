#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>

pid_t fork_pseudoterminal(int *parentFd, char *childName, size_t len,
                          const struct winsize *ws);
