#ifndef UI_BUILDER_H
#define UI_BUILDER_H

#include "ui_core.h"

WidgetInteraction ui_button(UIState *ui, String text);
void              ui_text(UIState *ui, String text);
WidgetInteraction ui_checkbox(UIState *ui, String text, b32 *b);
void              ui_spacing(UIState *ui, f32 amount);
void              ui_textbox(UIState *ui, StringBuilder *sb);
void              ui_begin_list(UIState *ui, String name);
void              ui_end_list(UIState *ui);
WidgetInteraction ui_selectable(UIState *ui, String text);

WidgetInteraction ui_begin_container(UIState *ui, String title, Vector2 size, RGBA32 color, UISizeKind size_kind, f32 child_padding);
void              ui_pop_container(UIState *ui);

#endif //UI_BUILDER_H
