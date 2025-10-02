#ifndef UI_BUILDER_H
#define UI_BUILDER_H

#include "ui_core.h"

/*
  TODO:
  - Put widget on same line
 */

WidgetInteraction ui_button(UIState *ui, String text);
WidgetInteraction ui_label(UIState *ui, String text);
WidgetInteraction ui_checkbox(UIState *ui, b32 *b);

#endif //UI_BUILDER_H
