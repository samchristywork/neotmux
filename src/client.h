#ifndef CLIENT_H
#define CLIENT_H

typedef enum Mode { MODE_NORMAL, MODE_CONTROL, MODE_CONTROL_STICKY } Mode;

int start_client(int sock);

#endif
