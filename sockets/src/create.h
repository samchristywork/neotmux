#ifndef CREATE_H
#define CREATE_H

#include "session.h"

Pane *add_pane(Window *window);
Window *add_window(Session *session, char *title);
Session *add_session(Neotmux *neotmux, char *title);

#endif
