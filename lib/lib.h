#ifndef LIB_H
#define LIB_H

enum {
  COLOR_TYPE_DEFAULT,
  COLOR_TYPE_256,
  COLOR_TYPE_RGB,
};

typedef struct Color {
  int type;
  int index;
  int r;
  int g;
  int b;
} Color;

typedef struct Cell {
  char value;
  Color fg;
  Color bg;
} Cell;

typedef struct Cursor {
  int x;
  int y;
} Cursor;

typedef struct State {
  Cell *cells;
  int width;
  int height;
  Cursor cursor;
} State;

State *create_state(int width, int height);
void print_cell(Cell cell);
void print_cells(State *state);
void send_input(State *state, char *input, int n);

#endif
