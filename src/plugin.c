#include <lua5.4/lauxlib.h>
#include <stdbool.h>
#include <unistd.h>

bool load_plugin(lua_State *L, const char *filename) {
  if (access(filename, F_OK) == -1) {
    fprintf(stderr, "File does not exist: %s\n", filename);
    return true;
  }

  if (luaL_loadfile(L, filename)) {
    fprintf(stderr, "Cannot load file: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    return false;
  }

  if (lua_pcall(L, 0, 0, 0)) {
    fprintf(stderr, "Cannot run file: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    return false;
  }

  return true;
}
