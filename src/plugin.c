#define _GNU_SOURCE

#include <dirent.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "session.h"

extern Neotmux *neotmux;

bool load_plugin(lua_State *L, const char *filename, const char *cwd) {
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");
  const char *path = lua_tostring(L, -1);
  lua_pop(L, 1);
  lua_pushfstring(L, "%s;%s/?.lua", path, cwd);
  lua_setfield(L, -2, "path");
  lua_pop(L, 1);

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

void load_plugins(int socket) {
  WRITE_LOG(LOG_INFO, socket, "Loading plugins");
  char *home = getenv("HOME");
  char plugins[PATH_MAX];
  snprintf(plugins, PATH_MAX, "%s/.ntmux/plugins", home);

  DIR *dir = opendir(plugins);
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
        size_t size = strlen(plugins) + strlen(entry->d_name) + 11;
        if (size > PATH_MAX) {
          fprintf(stderr, "Path too long\n");
          continue;
        }
        WRITE_LOG(LOG_INFO, socket, "Loading plugin: %s/%s", plugins,
                  entry->d_name);
        char path[size];
        snprintf(path, size, "%s/%s/init.lua", plugins, entry->d_name);

        char cwd[size];
        snprintf(cwd, size, "%s/%s", plugins, entry->d_name);
        load_plugin(neotmux->lua, path, cwd);
      }
    }

    closedir(dir);
  }
}

bool init_plugins(int socket) {
  neotmux->lua = luaL_newstate();
  luaL_openlibs(neotmux->lua);
  char *s = "print('Lua initialized')";
  luaL_dostring(neotmux->lua, s);

  lua_newtable(neotmux->lua);
  lua_setglobal(neotmux->lua, "ntmux");

  load_plugins(socket);

  char *home = getenv("HOME");
  char dotfile[PATH_MAX];
  snprintf(dotfile, PATH_MAX, "%s/.ntmux.lua", home);

  char cwd[PATH_MAX];
  getcwd(cwd, PATH_MAX);
  if (!load_plugin(neotmux->lua, dotfile, cwd)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
