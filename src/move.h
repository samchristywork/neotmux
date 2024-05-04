#ifndef MOVE_H
#define MOVE_H

#include "session.h"

enum direction { LEFT, RIGHT, UP, DOWN };

int move_active_pane(int d, Window *w);

#endif
