#include "ui_builder.h"
#include "game/ui/ui_core.h"
#include "game/ui/widget.h"

WidgetInteraction ui_text(UIState *ui, String text)
{
    Widget *widget = ui_core_create_widget(ui, V2_ZERO, debug_id_counter++);

    widget_add_flag(widget, WIDGET_TEXT);
    widget->text.string = text;
    widget->text.font = ui->current_style.font;
    widget->text.size = 12; // TODO: allow changing text size

    WidgetInteraction prev_interaction = ui_core_get_widget_interaction(ui, widget);
    return prev_interaction;
}

WidgetInteraction ui_button(UIState *ui, String text)
{
    Widget *widget = ui_core_create_widget(ui, v2(64, 32), debug_id_counter++);
    widget_add_flag(widget, WIDGET_CLICKABLE);
    widget_add_flag(widget, WIDGET_COLORED);

    // TODO: allow changing
    widget->size_kind = UI_SIZE_KIND_SUM_OF_CHILDREN;
    widget->child_padding = 12.0f;

    ui_core_push_as_container(ui, widget);
    ui_text(ui, text);
    ui_core_end_container(ui);

    WidgetInteraction prev_interaction = ui_core_get_widget_interaction(ui, widget);
    return prev_interaction;
}

WidgetInteraction ui_checkbox(UIState *ui, b32 *b)
{
    ASSERT(b);

    Widget *widget = ui_core_colored_box(ui, v2(12, 12), RGBA32_WHITE, debug_id_counter++);
    widget_add_flag(widget, WIDGET_CLICKABLE);
    widget->child_padding = 2.0f;

    ui_core_push_as_container(ui, widget);

    Widget *child_box = ui_core_colored_box(ui, v2(1.0f, 1.0f), RGBA32_BLUE, debug_id_counter++);
    child_box->size_kind = UI_SIZE_KIND_PERCENT_OF_PARENT;

    ui_core_end_container(ui);

    if (!(*b)) {
        widget_add_flag(child_box, WIDGET_HIDDEN);
    }

    WidgetInteraction prev_interaction = ui_core_get_widget_interaction(ui, widget);

    if (prev_interaction.clicked) {
        *b = !(*b);
    }

    return prev_interaction;
}
