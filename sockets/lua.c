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

bool get_global_boolean(lua_State *L, const char *name) {
  lua_getglobal(L, name);
  if (!lua_isboolean(L, -1)) {
    lua_pop(L, 1);
    return false;
  }
  bool result = lua_toboolean(L, -1);
  lua_pop(L, 1);
  return result;
}

char *get_global_string(lua_State *L, const char *name) {
  lua_getglobal(L, name);
  if (!lua_isstring(L, -1)) {
    lua_pop(L, 1);
    return NULL;
  }
  char *result = strdup(lua_tostring(L, -1));
  lua_pop(L, 1);
  return result;
}

bool exec_file(lua_State *L, const char *filename) {
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

int function_exists(lua_State *L, const char *function) {
  printf("Checking if function exists: %s\n", function);
  lua_getglobal(L, function);
  if (lua_isfunction(L, -1)) {
    printf("Function exists\n");
    lua_pop(L, 1);
    return 1;
  } else {
    printf("Function does not exist\n");
    lua_pop(L, 1);
    return 0;
  }
}

bool layout_function(lua_State *L, const char *function, int *col, int *row,
                     int *width, int *height, int index, int nPanes,
                     int winWidth, int winHeight) {
  lua_getglobal(L, function);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 1);
    return false;
  }

  lua_getglobal(L, function);
  lua_pushinteger(L, index);
  lua_pushinteger(L, nPanes);
  lua_pushinteger(L, winWidth);
  lua_pushinteger(L, winHeight);
  lua_call(L, 4, 4);

  *col = lua_tointeger(L, -4);
  *row = lua_tointeger(L, -3);
  *width = lua_tointeger(L, -2);
  *height = lua_tointeger(L, -1);

  lua_pop(L, 4);
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
      exec_file(lua, path);
    }
  }

  closedir(dir);
}
