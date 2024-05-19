#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "add.h"
#include "clipboard.h"
#include "layout.h"
#include "log.h"
#include "move.h"
#include "plugin.h"
#include "print_session.h"
#include "render.h"
#include "session.h"

extern Neotmux *neotmux;

#define swap_panes(w, a, b)                                                    \
  {                                                                            \
    Pane tmp = w->panes[a];                                                    \
    w->panes[a] = w->panes[b];                                                 \
    w->panes[b] = tmp;                                                         \
    w->current_pane = swap;                                                    \
    calculate_layout(w);                                                       \
  }

void handle_command(int socket, char *buf, int read_size);

// Category: Render
bool handle_render_command(int socket, char *cmd) {
  // Render the screen
  if (strcmp(cmd, "RenderScreen") == 0) {
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);
    render(socket, RENDER_SCREEN);
    gettimeofday(&end, NULL);

    static struct timeval programStart;
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    static int renderCount = 0;
    if (renderCount > 300) {
      renderCount = 0;
    }
    if (renderCount == 0) {
      gettimeofday(&programStart, NULL);
    }
    renderCount++;

    float elapsedTime = 0;
    elapsedTime += currentTime.tv_sec - programStart.tv_sec;
    elapsedTime += (currentTime.tv_usec - programStart.tv_usec) / 1000000.0;

    float renderTime = 0;
    renderTime = (end.tv_sec - start.tv_sec) * 1000.0;
    renderTime += (end.tv_usec - start.tv_usec) / 1000.0;

    WRITE_LOG(socket, "Render took %f ms", renderTime);
    WRITE_LOG(socket, "Renders: %d", renderCount);
    WRITE_LOG(socket, "Seconds: %f", elapsedTime);
    WRITE_LOG(socket, "Renders/Sec: %f", (float)renderCount / elapsedTime);

    // Render the status bar
  } else if (strcmp(cmd, "RenderBar") == 0) {
    render(socket, RENDER_BAR);

    // Re-render the current window
  } else if (strcmp(cmd, "ReRender") == 0) {
    Window *currentWindow = get_current_window(neotmux);
    currentWindow->rerender = true;
  } else {
    return false;
  }
  return true;
}

// Category: Directional
bool handle_directional_command(char *cmd) {
  // Swap the current pane with the pane to the left
  if (strcmp(cmd, "SwapLeft") == 0) {
    Window *w = get_current_window(neotmux);
    int swap = move_active_pane(LEFT, w);
    swap_panes(w, w->current_pane, swap);

    // Swap the current pane with the pane to the right
  } else if (strcmp(cmd, "SwapRight") == 0) {
    Window *w = get_current_window(neotmux);
    int swap = move_active_pane(RIGHT, w);
    swap_panes(w, w->current_pane, swap);

    // Swap the current pane with the pane above
  } else if (strcmp(cmd, "SwapUp") == 0) {
    Window *w = get_current_window(neotmux);
    int swap = move_active_pane(UP, w);
    swap_panes(w, w->current_pane, swap);

    // Swap the current pane with the pane below
  } else if (strcmp(cmd, "SwapDown") == 0) {
    Window *w = get_current_window(neotmux);
    int swap = move_active_pane(DOWN, w);
    swap_panes(w, w->current_pane, swap);

    // Move the current pane to the left
  } else if (strcmp(cmd, "Left") == 0) {
    Window *w = get_current_window(neotmux);
    w->current_pane = move_active_pane(LEFT, w);

    // Move the current pane to the right
  } else if (strcmp(cmd, "Right") == 0) {
    Window *w = get_current_window(neotmux);
    w->current_pane = move_active_pane(RIGHT, w);

    // Move the current pane up
  } else if (strcmp(cmd, "Up") == 0) {
    Window *w = get_current_window(neotmux);
    w->current_pane = move_active_pane(UP, w);

    // Move the current pane down
  } else if (strcmp(cmd, "Down") == 0) {
    Window *w = get_current_window(neotmux);
    w->current_pane = move_active_pane(DOWN, w);
  } else {
    return false;
  }
  return true;
}

// Category: Layout
bool handle_layout_command(char *cmd) {
  // Change the layout to even horizontal
  if (strcmp(cmd, "Even_Horizontal") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_EVEN_HORIZONTAL;
    calculate_layout(w);

    // Change the layout to even vertical
  } else if (strcmp(cmd, "Even_Vertical") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_EVEN_VERTICAL;
    calculate_layout(w);

    // Change the layout to main horizontal
  } else if (strcmp(cmd, "Main_Horizontal") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_MAIN_HORIZONTAL;
    calculate_layout(w);

    // Change the layout to main vertical
  } else if (strcmp(cmd, "Main_Vertical") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_MAIN_VERTICAL;
    calculate_layout(w);

    // Change the layout to tiled
  } else if (strcmp(cmd, "Tiled") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_TILED;
    calculate_layout(w);

    // Change the layout to custom
  } else if (strcmp(cmd, "Custom") == 0) {
    Window *w = get_current_window(neotmux);
    w->layout = LAYOUT_CUSTOM;
    calculate_layout(w);

    // Recalculate the layout
  } else if (strcmp(cmd, "Layout") == 0) {
    // TODO: Fix
    // if (session->current_window > 0 &&
    //    session->current_window < session->window_count) {

    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);
    //}
  } else {
    return false;
  }
  return true;
}

// Category: Debug
bool handle_debug_command(int socket, char *cmd) {
  // List all the sessions
  if (strcmp(cmd, "List") == 0) {
    print_sessions(neotmux, socket);

    // Print "Log" (used for debugging)
  } else if (strcmp(cmd, "Log") == 0) {
    printf("Log\n");

  } else {
    return false;
  }
  return true;
}

// Category: Control
bool handle_control_command(int socket, char *cmd) {
  // Initialize the window, session, and pane.
  if (strcmp(cmd, "Init") == 0) {
    load_plugins(socket);

    Session *s = add_session(neotmux, "Main");
    Window *w = add_window(s, "Main");

    if (neotmux->nCommands != 0) {
      for (int i = 0; i < neotmux->nCommands; i++) {
        add_pane(w, neotmux->commands[i]);
      }
    } else {
      int initial_panes = 3;
      lua_getglobal(neotmux->lua, "initial_panes");
      if (lua_isnumber(neotmux->lua, -1)) {
        initial_panes = lua_tonumber(neotmux->lua, -1);
      }
      lua_pop(neotmux->lua, 1);

      for (int i = 0; i < initial_panes; i++) {
        add_pane(w, NULL);
      }
    }

    // Reload the Lua plugins
  } else if (strcmp(cmd, "ReloadLua") == 0) {
    load_plugins(socket);

    // Quit the program
  } else if (strcmp(cmd, "Quit") == 0) {
    close(socket);
    exit(0);
  } else {
    return false;
  }
  return true;
}

// Category: Mode
bool handle_mode_command(int socket, char *cmd) {
  // Tell the server that the client is in Normal mode
  if (strcmp(cmd, "ModeNormal") == 0) {
    neotmux->mode = MODE_NORMAL;
    handle_command(socket, "RenderBar", 9);

    // Tell the server that the client is in Control mode
  } else if (strcmp(cmd, "ModeControl") == 0) {
    neotmux->mode = MODE_CONTROL;
    handle_command(socket, "RenderBar", 9);

    // Tell the server that the client is in Sticky Control mode
  } else if (strcmp(cmd, "ModeControlSticky") == 0) {
    neotmux->mode = MODE_CONTROL_STICKY;
    handle_command(socket, "RenderBar", 9);
  } else {
    return false;
  }
  return true;
}

// Category: Data
bool handle_data_command(char *cmd) {
  // Create a new window
  if (strcmp(cmd, "Create") == 0) {
    Session *session = get_current_session(neotmux);
    if (session != NULL) {
      Window *window;

      lua_getglobal(neotmux->lua, "default_title");
      if (!lua_isstring(neotmux->lua, -1)) {
        window = add_window(session, "New Window");
      } else {
        char *name = (char *)lua_tostring(neotmux->lua, -1);
        window = add_window(session, name);
        lua_pop(neotmux->lua, 1);
      }

      Window *w = get_current_window(neotmux);
      window->width = w->width;
      window->height = w->height;
      add_pane(window, NULL);
      session->current_window = session->window_count - 1;

      Window *currentWindow = get_current_window(neotmux);
      calculate_layout(currentWindow);
    }

    // Create a new window with a specified name
  } else if (memcmp(cmd, "CreateNamed", 11) == 0) {
    Session *session = get_current_session(neotmux);
    if (session != NULL) {
      char *title = strdup(cmd + 12);
      Window *window = add_window(session, title);

      Window *w = get_current_window(neotmux);
      window->width = w->width;
      window->height = w->height;
      add_pane(window, NULL);
      session->current_window = session->window_count - 1;

      Window *currentWindow = get_current_window(neotmux);
      calculate_layout(currentWindow);
    }

    // Rename the current window
  } else if (memcmp(cmd, "RenameWindow", 12) == 0) {
    Window *window = get_current_window(neotmux);
    free(window->title);
    char *title = strdup(cmd + 13);
    window->title = title;

    // Rename the current session
  } else if (memcmp(cmd, "RenameSession", 13) == 0) {
    Session *session = get_current_session(neotmux);
    free(session->title);
    char *title = strdup(cmd + 14);
    session->title = title;

    // Split the current pane
  } else if (strcmp(cmd, "VSplit") == 0) {
    Window *w = get_current_window(neotmux);
    add_pane(w, NULL);
    calculate_layout(w);
    w->current_pane = w->pane_count - 1;
    w->zoom = -1;

    // Split the current pane
  } else if (strcmp(cmd, "Split") == 0) {
    Window *w = get_current_window(neotmux);
    add_pane(w, NULL);
    calculate_layout(w);
    w->current_pane = w->pane_count - 1;
    w->zoom = -1;
  } else {
    return false;
  }
  return true;
}

// Category: Misc
bool handle_misc_command(int socket, char *cmd) {
  // Cycle the status bar
  if (strcmp(cmd, "CycleStatus") == 0) {
    neotmux->statusBarIdx++;
    handle_command(socket, "RenderBar", 9);

    // Select the status bar
  } else if (memcmp(cmd, "SelectStatus", 12) == 0) {
    neotmux->statusBarIdx = atoi(cmd + 13);
    handle_command(socket, "RenderBar", 9);

    // Switch to the next session
  } else if (strcmp(cmd, "Next") == 0) {
    Session *session = get_current_session(neotmux);
    session->current_window++;
    if (session->current_window >= session->window_count) {
      session->current_window = 0;
    }
    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);

    // Switch to the previous session
  } else if (strcmp(cmd, "Prev") == 0) {
    Session *session = get_current_session(neotmux);
    session->current_window--;
    if (session->current_window < 0) {
      session->current_window = session->window_count - 1;
    }
    Window *currentWindow = get_current_window(neotmux);
    calculate_layout(currentWindow);

    // Zoom the current pane
  } else if (strcmp(cmd, "Zoom") == 0) {
    // TODO: Fix the issue where mouse events are not being sent to the zoomed
    // window
    Window *w = get_current_window(neotmux);
    if (w->zoom == -1) {
      w->zoom = w->current_pane;
    } else {
      w->zoom = -1;
    }
    calculate_layout(w);

    // Scroll the current pane upwards
  } else if (strcmp(cmd, "ScrollUp") == 0) {
    Pane *p = get_current_pane(neotmux);
    p->process->scrolloffset += 1;
    Window *w = get_current_window(neotmux);
    w->rerender = true;

    // Scroll the current pane downwards
  } else if (strcmp(cmd, "ScrollDown") == 0) {
    Pane *p = get_current_pane(neotmux);
    p->process->scrolloffset -= 1;
    Window *w = get_current_window(neotmux);
    w->rerender = true;

    // Copy the current selection to the clipboard
  } else if (strcmp(cmd, "CopySelection") == 0) {
    Pane *pane = get_current_pane(neotmux);
    copy_selection_to_clipboard(pane);
    pane->selection.active = false;
  } else {
    return false;
  }
  return true;
}

void handle_command(int socket, char *buf, int read_size) {
  char cmd[read_size];
  memcpy(cmd, buf + 1, read_size - 1);
  cmd[read_size - 1] = '\0';

  WRITE_LOG(socket, "%s", cmd);

  if (handle_render_command(socket, cmd)) {
    return;
  } else if (handle_directional_command(cmd)) {
    return;
  } else if (handle_layout_command(cmd)) {
    return;
  } else if (handle_debug_command(socket, cmd)) {
    return;
  } else if (handle_control_command(socket, cmd)) {
    return;
  } else if (handle_mode_command(socket, cmd)) {
    return;
  } else if (handle_data_command(cmd)) {
    return;
    // TODO: Categorize misc commands
  } else if (handle_misc_command(socket, cmd)) {
    return;
  } else {
    WRITE_LOG(socket, "Unhandled command: %s", cmd);
  }
}
