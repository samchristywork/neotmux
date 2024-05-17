#ifndef ADD_H
#define ADD_H

#include "session.h"

Pane *add_pane(Window *window, char *cmd);
Window *add_window(Session *session, char *title);
Session *add_session(Neotmux *neotmux, char *title);

#endif
