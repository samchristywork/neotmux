#include <fcntl.h>
#include <lib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void init_cell(Cell *cell) {
  cell->value = ' ';

  cell->fg.type = COLOR_TYPE_NONE,
  cell->fg.index = 0;
  cell->fg.r = 0;
  cell->fg.g = 0;
  cell->fg.b = 0;

  cell->bg.type = COLOR_TYPE_NONE,
  cell->bg.index = 0;
  cell->bg.r = 0;
  cell->bg.g = 0;
  cell->bg.b = 0;
}

State *create_state(int width, int height) {
  State *state = malloc(sizeof(State));
  state->width = width;
  state->height = height;
  state->cursor.x = 0;
  state->cursor.y = 0;
  state->cells = malloc(sizeof(Cell) * width * height);
  for (int i = 0; i < width * height; i++) {
    init_cell(&state->cells[i]);
  }
  return state;
}

void write_color_fg(int fd, Color color) {
  char buf[16];
  int n;
  switch (color.type) {
    case COLOR_TYPE_INDEX:
      n = snprintf(buf, 16, "\033[38;5;%dm", color.index);
      break;
    case COLOR_TYPE_RGB:
      n = snprintf(buf, 16, "\033[38;2;%d;%d;%dm", color.r, color.g, color.b);
      break;
    default:
      return;
  }
  write(fd, buf, n);
}

void write_color_bg(int fd, Color color) {
  char buf[16];
  int n;
  switch (color.type) {
    case COLOR_TYPE_INDEX:
      n = snprintf(buf, 16, "\033[48;5;%dm", color.index);
      break;
    case COLOR_TYPE_RGB:
      n = snprintf(buf, 16, "\033[48;2;%d;%d;%dm", color.r, color.g, color.b);
      break;
    default:
      return;
  }
  write(fd, buf, n);
}

void reset_color_fg(int fd) {
  write(fd, "\033[39m", 5);
}

void reset_color_bg(int fd) {
  write(fd, "\033[49m", 5);
}

Color last_fg = {COLOR_TYPE_NONE, 0, 0, 0};
Color last_bg = {COLOR_TYPE_NONE, 0, 0, 0};

bool color_eq(Color a, Color b) {
  if (a.type != b.type) {
    return false;
  }
  switch (a.type) {
    case COLOR_TYPE_INDEX:
      return a.index == b.index;
    case COLOR_TYPE_RGB:
      return a.r == b.r && a.g == b.g && a.b == b.b;
    default:
      return true;
  }
}

void print_cell(int fd, Cell cell) {
  bool reset_fg = false;
  bool reset_bg = false;

  if (!color_eq(cell.fg, last_fg)) {
    if (last_fg.type != COLOR_TYPE_NONE) {
      reset_color_fg(fd);
    }
    write_color_fg(fd, cell.fg);
    last_fg = cell.fg;
    reset_fg = true;
  }

  if (!color_eq(cell.bg, last_bg)) {
    if (last_bg.type != COLOR_TYPE_NONE) {
      reset_color_bg(fd);
    }
    write_color_bg(fd, cell.bg);
    last_bg = cell.bg;
    reset_bg = true;
  }

  write(fd, &cell.value, 1);
}

void invert_colors(int fd) {
  write(fd, "\033[7m", 4);
}

void reset_colors(int fd) {
  write(fd, "\033[0m", 4);
}

void clear_screen(State *state) {
  for (int i = 0; i < state->width * state->height; i++) {
    init_cell(&state->cells[i]);
    state->cells[i].value = ' ';
  }
}

void clear_line(State *state) {
  for (int col = 0; col < state->width; col++) {
    state->cells[state->cursor.y * state->width + col].value = ' ';
  }
}

void print_cells(int fd, State *state) {
  for (int row = 0; row < state->height; row++) {
    for (int col = 0; col < state->width; col++) {
      if (state->cursor.x == col && state->cursor.y == row) {
        invert_colors(fd);
        print_cell(fd, state->cells[row * state->width + col]);
        reset_colors(fd);
      } else {
        print_cell(fd, state->cells[row * state->width + col]);
      }
    }
    printf("\n");
  }
}

void scroll_up(State *state) {
  for (int row = 1; row < state->height; row++) {
    for (int col = 0; col < state->width; col++) {
      Cell newCell = state->cells[row * state->width + col];
      state->cells[(row - 1) * state->width + col] = newCell;
    }
  }
  for (int col = 0; col < state->width; col++) {
    state->cells[(state->height - 1) * state->width + col].value = '.';
  }
  state->cursor.y--;
}

void send_input(State *state, char *input, int n) {
  for (int i = 0; i < n; i++) {
    int idx = state->cursor.y * state->width + state->cursor.x;
    Cell *cursor_cell = &state->cells[idx];

    if (input[i] == '\n') {
      state->cursor.x = 0;
      state->cursor.y++;
    } else {
      cursor_cell->value = input[i];
      state->cursor.x++;
      if (state->cursor.x == state->width) {
        state->cursor.x = 0;
        state->cursor.y++;
      }
    }

    if (state->cursor.y == state->height) {
      scroll_up(state);
    }
  }
}
