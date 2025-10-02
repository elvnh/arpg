#include "ui_builder.h"
#include "game/ui/ui_core.h"

WidgetInteraction ui_label(UIState *ui, String text)
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
    Widget *widget = ui_core_create_widget(ui, v2(64, 32), debug_id_counter++); // TODO: size based on text dims
    widget_add_flag(widget, WIDGET_CLICKABLE);
    widget_add_flag(widget, WIDGET_COLORED);

    ui_core_push_as_container(ui, widget);
    ui_label(ui, text);
    ui_core_end_container(ui);

    WidgetInteraction prev_interaction = ui_core_get_widget_interaction(ui, widget);
    return prev_interaction;
}
