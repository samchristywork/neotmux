#ifndef LIB_H
#define LIB_H

enum {
  COLOR_TYPE_NONE,
  COLOR_TYPE_INDEX,
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
void print_cells(int fd, State *state);
void send_input(State *state, char *input, int n);
void draw_cells(State *state, int x, int y);

#endif
