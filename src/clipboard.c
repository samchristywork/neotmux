#include <unistd.h>

#include "render.h"
#include "render_cell.h"
#include "session.h"

void copy_selection_to_clipboard(Pane *pane) {
  int fd[2];
  pipe(fd);

  for (int row = 0; row < pane->height; row++) {
    for (int col = pane->col; col < pane->col + pane->width; col++) {
      VTermPos pos;
      pos.row = row + pane->process->scrolloffset;
      pos.col = col - pane->col;
      if (is_within_selection(pane->selection, pos)) {
        VTermScreenCell cell = {0};
        VTermScreen *vts = pane->process->vts;
        vterm_screen_get_cell(vts, pos, &cell);

        if (cell.chars[0] == 0) {
          // write(fd[1], " ", 1);
        } else {
          for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i]; i++) {
            char bytes[6];
            int len = fill_utf8(cell.chars[i], bytes);
            bytes[len] = 0;
            write(fd[1], bytes, len);
          }
        }

        if (col == pane->col + pane->width - 1) {
          write(fd[1], "\n", 1);
        }
      }
    }
  }
  write(fd[1], "\n", 1);

  close(fd[1]);
  dup2(fd[0], STDIN_FILENO);
  close(fd[0]);
  system("xclip -selection clipboard");
}
