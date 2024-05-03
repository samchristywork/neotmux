#include <dirent.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int get_global_int(lua_State *L, const char *name) {
  lua_getglobal(L, name);
  if (!lua_isinteger(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  int result = lua_tointeger(L, -1);
  lua_pop(L, 1);
  return result;
}

// TODO: Allow for arguments to be passed to the function
void call_function(lua_State *L, const char *function) {
  lua_getglobal(L, function);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 1);
    return;
  } else if (lua_pcall(L, 0, 0, 0)) {
    fprintf(stderr, "Cannot run function: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  }
}

char *function_to_string(lua_State *L, const char *function) {
  lua_getglobal(L, function);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 1);
    return NULL;
  }
  lua_getglobal(L, function);
  lua_getglobal(L, function);
  lua_call(L, 0, 1);
  char *result = strdup(lua_tostring(L, -1));
  lua_pop(L, 1);
  return result;
}
