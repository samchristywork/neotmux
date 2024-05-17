#ifndef RENDER_H
#define RENDER_H

typedef enum RenderType {
  RENDER_NONE,
  RENDER_SCREEN,
  RENDER_BAR,
} RenderType;

void render(int fd, RenderType type);
bool is_within_selection(Selection selection, VTermPos pos);

#endif
