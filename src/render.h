#ifndef RENDER_H
#define RENDER_H

#include "session.h"

typedef enum RenderType {
  RENDER_NONE,
  RENDER_SCREEN,
  RENDER_BAR,
} RenderType;

void render(int fd, RenderType type);

#endif
