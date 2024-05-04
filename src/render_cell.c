#include <lua5.4/lauxlib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <vterm.h>

#include "session.h"

extern Neotmux *neotmux;

unsigned int calculate_utf8_length(long codepoint) {
  if (codepoint < 0x0000080) {
    return 1;
  } else if (codepoint < 0x0000800) {
    return 2;
  } else if (codepoint < 0x0010000) {
    return 3;
  } else if (codepoint < 0x0200000) {
    return 4;
  } else if (codepoint < 0x4000000) {
    return 5;
  } else {
    return 6;
  }
}

int fill_utf8(long codepoint, char *str) {
  int nbytes = calculate_utf8_length(codepoint);

  int b = nbytes;
  while (b > 1) {
    b--;
    str[b] = 0x80 | (codepoint & 0x3f);
    codepoint >>= 6;
  }

  switch (nbytes) {
  case 1:
    str[0] = (codepoint & 0x7f);
    break;
  case 2:
    str[0] = 0xc0 | (codepoint & 0x1f);
    break;
  case 3:
    str[0] = 0xe0 | (codepoint & 0x0f);
    break;
  case 4:
    str[0] = 0xf0 | (codepoint & 0x07);
    break;
  case 5:
    str[0] = 0xf8 | (codepoint & 0x03);
    break;
  case 6:
    str[0] = 0xfc | (codepoint & 0x01);
    break;
  }

  return nbytes;
}

bool compare_colors(VTermColor a, VTermColor b) {
  if (a.type != b.type) {
    return false;
  } else if (a.type == VTERM_COLOR_DEFAULT_FG ||
             a.type == VTERM_COLOR_DEFAULT_BG) {
    return true;
  } else if (VTERM_COLOR_IS_INDEXED(&a) && VTERM_COLOR_IS_INDEXED(&b)) {
    return a.indexed.idx == b.indexed.idx;
  } else if (VTERM_COLOR_IS_RGB(&a) && VTERM_COLOR_IS_RGB(&b)) {
    return a.rgb.red == b.rgb.red && a.rgb.green == b.rgb.green &&
           a.rgb.blue == b.rgb.blue;
  } else {
    return false;
  }
}

int has_lua_function(lua_State *L, const char *function) {
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

void write_rgb_color(int red, int green, int blue, int type) {
  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;2;%d;%d;%dm", type, red, green, blue);
  buf_write(buf, n);
}

// TODO: Interpolate into rgb colors
// TODO: Change from a function to static data
void write_indexed_color(int idx, int type) {
  static int has_interpolation_function = -1;
  if (has_interpolation_function == -1) {
    has_interpolation_function =
        has_lua_function(neotmux->lua, "interpolate_color");
  } else if (has_interpolation_function) {
    lua_getglobal(neotmux->lua, "interpolate_color");
    lua_pushinteger(neotmux->lua, idx);
    lua_call(neotmux->lua, 1, 4);

    bool use = lua_toboolean(neotmux->lua, -4);
    int red = lua_tointeger(neotmux->lua, -3);
    int green = lua_tointeger(neotmux->lua, -2);
    int blue = lua_tointeger(neotmux->lua, -1);

    lua_pop(neotmux->lua, 4);

    if (use) {
      write_rgb_color(red, green, blue, type);
      return;
    }
  }

  char buf[32];
  int n = snprintf(buf, 32, "\033[%d;5;%dm", type, idx);
  buf_write(buf, n);
}

void render_cell(VTermScreenCell cell) {
  if (cell.attrs.bold != neotmux->prevCell.attrs.bold) {
    if (cell.attrs.bold) {
      buf_write("\033[1m", 4); // Enable bold
    } else {
      buf_write("\033[22m", 5); // Disable bold
    }
    neotmux->prevCell.attrs.bold = cell.attrs.bold;
  }

  if (cell.attrs.reverse != neotmux->prevCell.attrs.reverse) {
    if (cell.attrs.reverse) {
      buf_write("\033[7m", 4); // Enable reverse
    } else {
      buf_write("\033[27m", 5); // Disable reverse
    }
    neotmux->prevCell.attrs.reverse = cell.attrs.reverse;
  }

  if (cell.attrs.underline != neotmux->prevCell.attrs.underline) {
    if (cell.attrs.underline) {
      buf_write("\033[4m", 4); // Enable underline
    } else {
      buf_write("\033[24m", 5); // Disable underline
    }
    neotmux->prevCell.attrs.underline = cell.attrs.underline;
  }

  if (cell.attrs.italic != neotmux->prevCell.attrs.italic) {
    if (cell.attrs.italic) {
      buf_write("\033[3m", 4); // Enable italic
    } else {
      buf_write("\033[23m", 5); // Disable italic
    }
    neotmux->prevCell.attrs.italic = cell.attrs.italic;
  }

  if (cell.attrs.strike != neotmux->prevCell.attrs.strike) {
    if (cell.attrs.strike) {
      buf_write("\033[9m", 4); // Enable strike
    } else {
      buf_write("\033[29m", 5); // Disable strike
    }
    neotmux->prevCell.attrs.strike = cell.attrs.strike;
  }

  if (!compare_colors(cell.bg, neotmux->prevCell.bg)) {
    if (cell.bg.type == VTERM_COLOR_DEFAULT_BG) {
    } else if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
      write_indexed_color(cell.bg.indexed.idx, 48);
      neotmux->prevCell.bg = cell.bg;
    } else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
      write_rgb_color(cell.bg.rgb.red, cell.bg.rgb.green, cell.bg.rgb.blue, 48);
      neotmux->prevCell.bg = cell.bg;
    }
  }

  if (!compare_colors(cell.fg, neotmux->prevCell.fg)) {
    if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
      write_indexed_color(cell.fg.indexed.idx, 38);
    } else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
      write_rgb_color(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue, 38);
    }
    neotmux->prevCell.fg = cell.fg;
  }

  if (cell.chars[0] == 0) {
    buf_write(" ", 1);
  } else {
    for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i]; i++) {
      char bytes[6];
      int len = fill_utf8(cell.chars[i], bytes);
      bytes[len] = 0;
      buf_write(bytes, len);
    }
  }
}
