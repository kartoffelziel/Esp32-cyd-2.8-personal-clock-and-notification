#pragma once
#include "text.h"

enum IconKind { ICON_GEAR, ICON_POWER };

// Round button with an anti-aliased vector icon, composed in one pass over the (gradient)
// background so it looks smooth at any position. `size` is the outer square (44 looks right).
void drawIconButton(IconKind kind, int x, int y, int size, const Bg &bg, bool pressed);
