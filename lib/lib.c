#include <lib.h>
#include <stdio.h>
#include <stdlib.h>

void init_cell(Cell *cell) {
  cell->value = '?';
  cell->fg.type = 0;
  cell->fg.index = 0;
  cell->fg.r = 0;
  cell->fg.g = 0;
  cell->fg.b = 0;
  cell->bg.type = 0;
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
    state->cells[i].value = '.';
  }
  return state;
}

void print_cell(Cell cell) { printf("%c", cell.value); }

void invert_colors() { printf("\033[7m"); }

void reset_colors() { printf("\033[0m"); }

void print_cells(State *state) {
  for (int row = 0; row < state->height; row++) {
    for (int col = 0; col < state->width; col++) {
      if (state->cursor.x == col && state->cursor.y == row) {
        invert_colors();
        print_cell(state->cells[row * state->width + col]);
        reset_colors();
      } else {
        print_cell(state->cells[row * state->width + col]);
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
