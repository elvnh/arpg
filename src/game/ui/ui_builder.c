#include "ui_builder.h"
#include "ui_core.h"
#include "widget.h"
#include "base/rgba.h"

// ID is only needed if interacting with widget

static Widget *ui_internal_text(UIState *ui, String text, Vector2 offset, RGBA32 color)
{
    Widget *widget = ui_core_create_widget(ui, V2_ZERO, UI_NULL_WIDGET_ID);

    widget_add_flag(widget, WIDGET_TEXT);

    widget->text.string = text;
    widget->text.font = ui->current_style.font;
    widget->text.size = 18; // TODO: allow changing text size
    widget->color = color;
    widget->offset_from_parent = offset;

    return widget;
}

void ui_text(UIState *ui, String text)
{
    ui_internal_text(ui, text, V2_ZERO, RGBA32_WHITE);
}

WidgetInteraction ui_button(UIState *ui, String text)
{
    Widget *widget = ui_core_colored_box(ui, v2(0, 32.0f), RGBA32_BLUE, ui_core_hash_string(text));
    widget_add_flag(widget, WIDGET_CLICKABLE);

    // TODO: allow changing
    widget->horizontal_size_kind = UI_SIZE_KIND_SUM_OF_CHILDREN;
    widget->vertical_size_kind = UI_SIZE_KIND_ABSOLUTE;

    widget->child_padding = 8.0f;

    // TODO: center_next_widget
    ui_core_push_container(ui, widget);
    ui_internal_text(ui, text, v2(0, widget->preliminary_size.y / 2.0f - widget->child_padding * 2), RGBA32_WHITE);
    ui_core_pop_container(ui);

    WidgetInteraction prev_interaction = ui_core_get_widget_interaction(ui, widget);
    return prev_interaction;
}

WidgetInteraction ui_checkbox(UIState *ui, String text, b32 *b)
{
    // TODO: fix vertical alignment with text
    ASSERT(b);

    Widget *widget = ui_core_colored_box(ui, v2(12, 12), RGBA32_WHITE, ui_core_hash_string(text));
    widget_add_flag(widget, WIDGET_CLICKABLE);

    widget->child_padding = 2.0f;

    ui_core_push_container(ui, widget);

    Widget *child_box = ui_core_colored_box(ui, v2(1.0f, 1.0f), RGBA32_BLUE, UI_NULL_WIDGET_ID);
    child_box->horizontal_size_kind = UI_SIZE_KIND_PERCENT_OF_PARENT;
    child_box->vertical_size_kind = UI_SIZE_KIND_PERCENT_OF_PARENT;

    ui_pop_container(ui);

    if (!(*b)) {
        widget_add_flag(child_box, WIDGET_HIDDEN);
    }

    ui_core_same_line(ui);
    ui_text(ui, text);

    WidgetInteraction prev_interaction = ui_core_get_widget_interaction(ui, widget);

    if (prev_interaction.clicked) {
        *b = !(*b);
    }

    return prev_interaction;
}

void ui_spacing(UIState *ui, f32 amount)
{
    Widget *widget = ui_core_create_widget(ui, v2(amount, amount), UI_NULL_WIDGET_ID);
    widget_add_flag(widget, WIDGET_HIDDEN);

    if (widget->layout_direction == UI_LAYOUT_VERTICAL) {
        widget->preliminary_size.x = 1.0f;
    } else {
        widget->preliminary_size.y = 1.0f;
    }
}

void ui_textbox(UIState *ui, StringBuilder *sb)
{
    Widget *widget = ui_core_colored_box(ui, v2(128, 32), RGBA32_WHITE, (u64)sb);
    widget->horizontal_size_kind = UI_SIZE_KIND_ABSOLUTE;
    widget->vertical_size_kind = UI_SIZE_KIND_SUM_OF_CHILDREN;

    widget_add_flag(widget, WIDGET_CLICKABLE);
    widget_add_flag(widget, WIDGET_TEXT_INPUT);
    widget_add_flag(widget, WIDGET_STAY_ACTIVE);

    widget->text_input_buffer = sb;

    ui_core_push_container(ui, widget);
    ui_internal_text(ui, sb->buffer, V2_ZERO, RGBA32_BLACK);
    ui_core_pop_container(ui);
}

WidgetInteraction ui_begin_container(UIState *ui, String title, Vector2 size, UISizeKind size_kind, f32 child_padding)
{
    Widget *widget = ui_core_create_widget(ui, size, ui_core_hash_string(title));

    widget->child_padding = child_padding;
    widget->horizontal_size_kind = size_kind;
    widget->vertical_size_kind = size_kind;

    ui_core_push_container(ui, widget);

    WidgetInteraction prev_interaction = ui_core_get_widget_interaction(ui, widget);
    return prev_interaction;
}

void ui_pop_container(UIState *ui)
{
    ui_core_pop_container(ui);
}
