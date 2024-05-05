#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>

#include "move.h"
#include "session.h"

extern Neotmux *neotmux;

void push_pane(lua_State *lua, Pane *pane) {
  lua_newtable(lua);
  lua_pushinteger(lua, pane->col);
  lua_setfield(lua, -2, "col");
  lua_pushinteger(lua, pane->row);
  lua_setfield(lua, -2, "row");
  lua_pushinteger(lua, pane->width);
  lua_setfield(lua, -2, "width");
  lua_pushinteger(lua, pane->height);
  lua_setfield(lua, -2, "height");
}

int call_lua_movement_function(char *functionName, Window *w) {
  Pane *currentPane = &w->panes[w->current_pane];
  lua_State *lua = neotmux->lua;
  lua_getglobal(lua, functionName);

  // Pane
  push_pane(lua, currentPane);

  // Panes
  lua_newtable(lua);
  for (int i = 0; i < w->pane_count; i++) {
    Pane *pane = &w->panes[i];
    push_pane(lua, pane);
    lua_rawseti(lua, -2, i + 1);
  }

  // Width
  lua_pushinteger(lua, w->width);

  // Height
  lua_pushinteger(lua, w->height);

  lua_call(lua, 4, 1);

  int returnValue = lua_tointeger(lua, -1);
  lua_pop(lua, 1);
  return returnValue;
}

int move_active_pane(Direction d, Window *w) {
  switch (d) {
  case LEFT: {
    return call_lua_movement_function("handleLeft", w);
  }
  case RIGHT: {
    return call_lua_movement_function("handleRight", w);
  }
  case UP: {
    return call_lua_movement_function("handleUp", w);
  }
  case DOWN: {
    return call_lua_movement_function("handleDown", w);
  }
  default:
    return -1;
  }
}
