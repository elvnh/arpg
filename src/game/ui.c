#include <string.h>

#include "ui.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "base/sl_list.h"
#include "game/renderer/render_batch.h"
#include "input.h"

#define FRAME_WIDGET_TABLE_SIZE 1024

FrameWidgetTable frame_widget_table_create(LinearArena *arena)
{
    FrameWidgetTable result = {0};
    result.entry_table_size = FRAME_WIDGET_TABLE_SIZE;
    result.entries = la_allocate_array(arena, WidgetList, result.entry_table_size);
    result.arena = la_create(la_allocator(arena), FRAME_WIDGET_TABLE_SIZE * SIZEOF(*result.entries));

    return result;
}

static void frame_widget_table_push(FrameWidgetTable *table, Widget *widget)
{
    ssize index = hash_index(widget->id, table->entry_table_size);
    sl_list_push_back(&table->entries[index], widget);
}

static Widget *frame_widget_table_find(FrameWidgetTable *table, WidgetID id)
{
    ssize index = hash_index(id, table->entry_table_size);

    WidgetList *list = &table->entries[index];

    Widget *result = 0;
    for (Widget *widget = sl_list_head(list); widget; widget = sl_list_next(widget)) {
        if (widget->id == id) {
            result = widget;
            break;
        }
    }

    return result;
}

void ui_initialize(UIState *ui, LinearArena *arena)
{
    ui->previous_frame_widgets = frame_widget_table_create(arena);
    ui->current_frame_widgets = frame_widget_table_create(arena);
}

void ui_begin_frame(UIState *ui)
{
    {
        FrameWidgetTable tmp = ui->previous_frame_widgets;
        ui->previous_frame_widgets = ui->current_frame_widgets;
        ui->current_frame_widgets = tmp;

        zero_array(ui->current_frame_widgets.entries, ui->current_frame_widgets.entry_table_size);
    }

    ui->root_widget = 0;
}

static void calculate_widget_layout(Widget *widget, Vector2 offset)
{
    (void)widget;
    widget->final_position = offset;
}

static Rectangle get_widget_bounding_box(Widget *widget)
{
    Rectangle result = {widget->final_position, widget->size};

    return result;
}

static void calculate_widget_interactions(Widget *widget, const Input *input)
{
    if (widget->flags & WIDGET_CLICKABLE) {
        ASSERT(widget->size.x);
        ASSERT(widget->size.y);

        Rectangle rect = get_widget_bounding_box(widget);
        if (rect_contains_point(rect, input->mouse_position) && input_is_key_released(input, KEY_W)) {
            widget->interaction_state.clicked = true;
        }
    }
}

static void render_widget(UIState *ui, Widget *widget, RenderBatch *rb, const AssetList *assets, ssize depth)
{
    (void)widget;
    (void)rb;

    // TODO: render differently if clicked
    Rectangle rect = {widget->final_position, widget->size};
    rb_push_rect(rb, &ui->current_frame_widgets.arena, rect, RGBA32_BLUE, assets->shader2, depth);
}

void ui_end_frame(UIState *ui, const struct Input *input, RenderBatch *rb, const AssetList *assets)
{
    if (ui->root_widget) {
        calculate_widget_layout(ui->root_widget, V2_ZERO);
        calculate_widget_interactions(ui->root_widget, input);
        render_widget(ui, ui->root_widget, rb, assets, 0);
    }
}

static Widget *widget_create(UIState *ui, WidgetID id)
{
    Widget *widget = la_allocate_item(&ui->current_frame_widgets.arena, Widget);
    widget->id = id;

    frame_widget_table_push(&ui->current_frame_widgets, widget);

    if (!ui->root_widget) {
        ui->root_widget = widget;
    }

    return widget;
}

WidgetInteraction get_previous_frame_interaction(UIState *ui, WidgetID id)
{
    WidgetInteraction result = {0};
    Widget *widget = frame_widget_table_find(&ui->previous_frame_widgets, id);

    if (widget) {
        result = widget->interaction_state;
    }

    return result;
}

WidgetInteraction ui_button(UIState *ui, String text, Vector2 position)
{
    (void)text;

    Widget *widget = widget_create(ui, 0);
    widget->flags |= WIDGET_CLICKABLE;
    widget->offset = position;
    widget->size = v2(128, 64); // TODO: size based on text dims

    // TODO: anonymous widgets for widgets that don't need state?
#if 0
    Widget *text_widget = widget_create(ui, 1);
    text_widget->flags |=
    sl_list_push_back(&widget->children, );
#endif

    WidgetInteraction prev_interaction = get_previous_frame_interaction(ui, widget->id);

    return prev_interaction;
}
