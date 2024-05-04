#ifndef PLUGIN_H
#define PLUGIN_H

#include <lua5.4/lua.h>

bool load_plugin(lua_State *L, const char *filename);

#endif
