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

WidgetFrameTable widget_frame_table_create(LinearArena *arena)
{
    WidgetFrameTable result = {0};
    result.entry_table_size = FRAME_WIDGET_TABLE_SIZE;
    result.entries = la_allocate_array(arena, WidgetList, result.entry_table_size);
    result.arena = la_create(la_allocator(arena), FRAME_WIDGET_TABLE_SIZE * SIZEOF(*result.entries));

    return result;
}

static void widget_frame_table_push(WidgetFrameTable *table, Widget *widget)
{
    ssize index = hash_index(widget->id, table->entry_table_size);
    sl_list_push_back(&table->entries[index], widget);
}

static Widget *widget_frame_table_find(WidgetFrameTable *table, WidgetID id)
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
    ui->previous_frame_widgets = widget_frame_table_create(arena);
    ui->current_frame_widgets = widget_frame_table_create(arena);
}

void ui_begin_frame(UIState *ui)
{
    {
        WidgetFrameTable tmp = ui->previous_frame_widgets;
        ui->previous_frame_widgets = ui->current_frame_widgets;
        ui->current_frame_widgets = tmp;

        zero_array(ui->current_frame_widgets.entries, ui->current_frame_widgets.entry_table_size);
    }

    ui->root_widget = 0;
}

static void calculate_widget_layout(Widget *widget, Vector2 offset)
{
    (void)widget;
    widget->final_position = v2_add(widget->offset_from_parent, offset);
    widget->final_size = widget->preliminary_size;
}

static Rectangle get_widget_bounding_box(Widget *widget)
{
    Rectangle result = {widget->final_position, widget->final_size};

    return result;
}

static b32 widget_has_flag(const Widget *widget, WidgetFlag flag)
{
    b32 result = (widget->flags & flag) != 0;
    return result;
}

static void widget_add_flag(Widget *widget, WidgetFlag flag)
{
    widget->flags |= flag;
}

static void calculate_widget_interactions(Widget *widget, const Input *input)
{
    if (widget_has_flag(widget, WIDGET_CLICKABLE)) {
        ASSERT(widget->final_size.x);
        ASSERT(widget->final_size.y);

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
    if (widget_has_flag(widget, WIDGET_COLORED)) {
        Rectangle rect = get_widget_bounding_box(widget);
        rb_push_rect(rb, &ui->current_frame_widgets.arena, rect, RGBA32_BLUE, assets->shader2, depth);
    }
}

void ui_end_frame(UIState *ui, const struct Input *input, RenderBatch *rb, const AssetList *assets)
{
    if (ui->root_widget) {
        calculate_widget_layout(ui->root_widget, V2_ZERO);
        calculate_widget_interactions(ui->root_widget, input);
        render_widget(ui, ui->root_widget, rb, assets, 0);
    }
}

static Widget *widget_create(UIState *ui, Vector2 offset, Vector2 size, WidgetID id)
{
    Widget *widget = la_allocate_item(&ui->current_frame_widgets.arena, Widget);
    widget->id = id;
    widget->offset_from_parent = offset;
    widget->preliminary_size = size;

    widget_frame_table_push(&ui->current_frame_widgets, widget);

    if (!ui->root_widget) {
        ui->root_widget = widget;
    }

    return widget;
}

WidgetInteraction get_previous_frame_interaction(UIState *ui, WidgetID id)
{
    WidgetInteraction result = {0};
    Widget *widget = widget_frame_table_find(&ui->previous_frame_widgets, id);

    if (widget) {
        result = widget->interaction_state;
    }

    return result;
}

WidgetInteraction ui_button(UIState *ui, String text, Vector2 position)
{
    (void)text;

    Widget *widget = widget_create(ui, position, v2(128, 64), 0); // TODO: size based on text dims
    widget_add_flag(widget, WIDGET_CLICKABLE);
    widget_add_flag(widget, WIDGET_COLORED);

    WidgetInteraction prev_interaction = get_previous_frame_interaction(ui, widget->id);

    return prev_interaction;
}
