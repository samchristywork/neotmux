#ifndef LUA_H
#define LUA_H

#include <lua5.4/lua.h>
#include <stdbool.h>

int get_global_int(lua_State *L, const char *name);
char *get_global_string(lua_State *L, const char *name);
bool get_global_boolean(lua_State *L, const char *name);
bool exec_file(lua_State *L, const char *filename);
void call_function(lua_State *L, const char *function);
char *function_to_string(lua_State *L, const char *function);
int function_exists(lua_State *L, const char *function);
bool layout_function(lua_State *L, const char *function, int *col, int *row,
                     int *width, int *height, int index, int nPanes,
                     int winWidth, int winHeight);
void load_plugins(lua_State *lua);

#endif
