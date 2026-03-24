#ifndef UI_BUILDER_H
#define UI_BUILDER_H

#include "ui_core.h"

WidgetInteraction ui_button(UIState *ui, String text);
WidgetInteraction ui_non_interactible_button(UIState *ui, String text);
void ui_text(UIState *ui, String text);
WidgetInteraction ui_checkbox(UIState *ui, String text, b32 *b);
void ui_spacing(UIState *ui, f32 amount);
void ui_textbox(UIState *ui, StringBuilder *sb);
void ui_begin_list(UIState *ui, String name);
void ui_end_list(UIState *ui);
WidgetInteraction ui_selectable(UIState *ui, String text);
void ui_begin_mouse_menu(UIState *ui, Vector2 mouse_pos);
void ui_end_mouse_menu(UIState *ui);

void ui_begin_container(UIState *ui, Vector2 size,
    UISizeKind size_kind, f32 child_padding);
void ui_pop_container(UIState *ui);

WidgetInteraction ui_begin_menu(UIState *ui, Vector2 size, String name,
    UISizeKind size_kind, f32 child_padding);
void ui_pop_menu(UIState *ui);

#endif //UI_BUILDER_H
