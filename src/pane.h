#ifndef PANE_H
#define PANE_H

#include "session.h"

char *get_lua_string(lua_State *L, const char *name);
void add_process_to_pane(Pane *pane);

#endif
