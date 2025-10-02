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
        la_reset(&ui->current_frame_widgets.arena);
    }

    ui->root_widget = 0;
}

static void calculate_widget_layout(Widget *widget, Vector2 offset)
{
    Vector2 final_position =  v2_add(widget->offset_from_parent, offset);
    widget->final_size = widget->preliminary_size;
    widget->final_position = final_position;

    for (Widget *child = sl_list_head(&widget->children); child; child = sl_list_next(child)) {
        // TODO: layout in row
        calculate_widget_layout(child, final_position);
    }
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
    for (Widget *child = sl_list_head(&widget->children); child; child = sl_list_next(child)) {
        calculate_widget_interactions(child, input);
    }

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
    // TODO: depth isn't needed once a stable sort is implemented for render commands
    // TODO: render differently if clicked
    RGBA32 color = {0, 1.0f, 0.1f, 0.3f};

    if (widget_has_flag(widget, WIDGET_COLORED)) {
        color = RGBA32_BLUE;
    }

    Rectangle rect = get_widget_bounding_box(widget);
    rb_push_rect(rb, &ui->current_frame_widgets.arena, rect, color, assets->shader2, depth);

    for (Widget *child = sl_list_head(&widget->children); child; child = sl_list_next(child)) {
        render_widget(ui, child, rb, assets, depth + 1);
    }
}

void ui_end_frame(UIState *ui, const struct Input *input, RenderBatch *rb, const AssetList *assets)
{
    ASSERT(sl_list_is_empty(&ui->container_stack));

    if (ui->root_widget) {
        calculate_widget_layout(ui->root_widget, V2_ZERO);
        calculate_widget_interactions(ui->root_widget, input);
        render_widget(ui, ui->root_widget, rb, assets, 0);
    }
}

static LinearArena *get_frame_arena(UIState *ui)
{
    LinearArena *result = &ui->current_frame_widgets.arena;

    return result;
}

static Widget *get_top_container(UIState *ui)
{
    Widget *result = sl_list_head(&ui->container_stack);

    return result;
}

static void widget_add_to_children(Widget *widget, Widget *child)
{
    sl_list_push_front(&widget->children, child);
}

static Widget *widget_create(UIState *ui, Vector2 offset, Vector2 size, WidgetID id)
{
    Widget *widget = la_allocate_item(get_frame_arena(ui), Widget);
    widget->id = id;
    widget->offset_from_parent = offset;
    widget->preliminary_size = size;

    widget_frame_table_push(&ui->current_frame_widgets, widget);

    if (!ui->root_widget) {
        ui->root_widget = widget;
    }

    Widget *top_container = get_top_container(ui);
    if (top_container) {
        widget_add_to_children(top_container, widget);
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

void ui_begin_container(UIState *ui, Vector2 offset, Vector2 size, LayoutKind layout)
{
    ASSERT(layout == UI_LAYOUT_VERTICAL);
    Widget *widget = widget_create(ui, offset, size, 1234);
    widget->layout_kind = layout;

    sl_list_push_front(&ui->container_stack, widget);
}

void ui_end_container(UIState *ui)
{
    ASSERT(!sl_list_is_empty(&ui->container_stack));
    sl_list_pop(&ui->container_stack);
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
