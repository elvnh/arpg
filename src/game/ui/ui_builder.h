#ifndef UI_BUILDER_H
#define UI_BUILDER_H

#include "ui_core.h"

WidgetInteraction ui_button(UIState *ui, String text);
void              ui_text(UIState *ui, String text);
WidgetInteraction ui_checkbox(UIState *ui, String text, b32 *b);

#endif //UI_BUILDER_H
