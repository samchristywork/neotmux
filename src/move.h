#ifndef MOVE_H
#define MOVE_H

#include "session.h"

typedef enum direction { LEFT, RIGHT, UP, DOWN } Direction;

// TODO: Rename
int move_active_pane(Direction d, Window *w);

#endif
