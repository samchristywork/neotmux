#include <dirent.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool execute_lua_file(lua_State *L, const char *filename) {
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

void load_plugins(lua_State *lua) {
  printf("Loading plugins\n");
  char *home = getenv("HOME");
  char plugins[PATH_MAX];
  snprintf(plugins, PATH_MAX, "%s/.ntmux/plugins", home);

  DIR *dir = opendir(plugins);
  if (!dir) {
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
      printf("Loading plugin: %s\n", entry->d_name);
      char path[PATH_MAX];
      snprintf(path, PATH_MAX, "%s/%s/init.lua", plugins, entry->d_name);
      execute_lua_file(lua, path);
    }
  }

  closedir(dir);
}
