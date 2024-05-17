#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "session.h"

void copy_selection_to_clipboard(Pane *pane);
bool is_within_selection(Selection selection, VTermPos pos);

#endif
