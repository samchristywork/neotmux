#ifndef PLUGIN_H
#define PLUGIN_H

#include <lua5.4/lua.h>
#include <stdbool.h>

bool exec_file(lua_State *L, const char *filename);
void load_plugins(lua_State *lua);

#endif
