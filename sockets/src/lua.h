#ifndef LUA_H
#define LUA_H

#include <lua5.4/lua.h>
#include <stdbool.h>

int get_lua_int(lua_State *L, const char *name);
bool execute_lua_file(lua_State *L, const char *filename);
char *apply_lua_string_function(lua_State *L, const char *function);
void load_plugins(lua_State *lua);

#endif
