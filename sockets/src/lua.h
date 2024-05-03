#ifndef LUA_H
#define LUA_H

#include <lua5.4/lua.h>
#include <stdbool.h>

int get_global_int(lua_State *L, const char *name);
bool exec_file(lua_State *L, const char *filename);
void call_function(lua_State *L, const char *function);
char *function_to_string(lua_State *L, const char *function);
void load_plugins(lua_State *lua);

#endif
