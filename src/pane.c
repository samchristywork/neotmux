#include <stdio.h>
#include <unistd.h>

#include "pty.h"
#include "session.h"

extern Neotmux *neotmux;

bool get_lua_boolean(lua_State *L, const char *name, bool default_value) {
  lua_getglobal(L, name);
  if (!lua_isboolean(L, -1)) {
    lua_pop(L, 1);
    return default_value;
  }
  bool result = lua_toboolean(L, -1);
  lua_pop(L, 1);
  return result;
}

char *get_lua_string(lua_State *L, const char *name, char *default_value) {
  lua_getglobal(L, name);
  if (!lua_isstring(L, -1)) {
    lua_pop(L, 1);
    return default_value;
  }
  char *result = strdup(lua_tostring(L, -1));
  lua_pop(L, 1);
  return result;
}

// TODO: Make sure we are accessing the right memory
int handle_term_prop(VTermProp prop, VTermValue *val, void *user) {
  Process *process = (Process *)user;
  if (prop == VTERM_PROP_CURSORVISIBLE) {
    process->cursor.visible = val->boolean;
  } else if (prop == VTERM_PROP_CURSORSHAPE) {
    process->cursor.shape = val->number;
  } else if (prop == VTERM_PROP_MOUSE) {
    process->cursor.mouse_active = val->number;
  }

  return 0;
}

int handle_push_line(int cols, const VTermScreenCell *cells, void *user) {
  // TODO: Copy these lines into scrollback buffer
  fprintf(neotmux->log, "Cols: %d\n", cols);
  for (int i = 0; i < cols; i++) {
    fprintf(neotmux->log, "%c", cells[i].chars[0]);
  }
  fprintf(neotmux->log, "\n");
  return 0;
}

void initialize_vterm_instance(VTerm **vt, VTermScreen **vts, int h, int w,
                               Pane *pane) {
  *vt = vterm_new(h, w);
  if (!vt) {
    exit(EXIT_FAILURE);
  }

  *vts = vterm_obtain_screen(*vt);

  // TODO: Implement callbacks
  static VTermScreenCallbacks callbacks;
  callbacks.damage = NULL;
  callbacks.moverect = NULL;
  callbacks.movecursor = NULL;
  callbacks.settermprop = handle_term_prop;
  callbacks.bell = NULL;
  callbacks.resize = NULL;
  callbacks.sb_pushline = handle_push_line; // TODO: Use for scrolling
  callbacks.sb_popline = NULL;              // TODO: Use for scrolling

  vterm_set_utf8(*vt, 1);
  vterm_screen_reset(*vts, 1);
  vterm_screen_enable_altscreen(*vts, 1);
  vterm_screen_set_callbacks(*vts, &callbacks, pane->process);
}

void add_process_to_pane(Pane *pane) {
  bool keepDir = get_lua_boolean(neotmux->lua, "keep_dir", false);
  struct winsize ws = {0};
  ws.ws_col = pane->width;
  ws.ws_row = pane->height;

  char childName[PATH_MAX];
  pid_t childPid =
      fork_pseudoterminal(&pane->process->fd, childName, PATH_MAX, &ws);
  if (childPid == -1) {
    exit(EXIT_FAILURE);
  } else if (childPid == 0) { // Child
    if (keepDir) {
      Pane *pane = get_current_pane(neotmux);
      char cwd[PATH_MAX] = {0};
      char link[PATH_MAX] = {0};
      sprintf(link, "/proc/%d/cwd", pane->process->pid);
      if (readlink(link, cwd, sizeof(cwd)) != -1) {
        chdir(cwd);
      }
    }

    char *shell = get_lua_string(neotmux->lua, "shell", NULL);
    if (!shell) {
      shell = getenv("SHELL");
    }

    execlp(shell, shell, (char *)NULL);
    exit(EXIT_FAILURE);
  }

  pane->process->pid = childPid;
  pane->process->name = malloc(strlen(childName) + 1);
  strcpy(pane->process->name, childName);

  initialize_vterm_instance(&pane->process->vt, &pane->process->vts, ws.ws_row,
                            ws.ws_col, pane);
}
