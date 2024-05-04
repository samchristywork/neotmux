#include <dirent.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <stdbool.h>
#include <unistd.h>

int get_lua_int(lua_State *L, const char *name) {
  lua_getglobal(L, name);
  if (!lua_isinteger(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  int result = lua_tointeger(L, -1);
  lua_pop(L, 1);
  return result;
}
