#include <string.h>

#include "ui_core.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "base/sl_list.h"
#include "game/renderer/render_batch.h"
#include "input.h"

#define FRAME_WIDGET_TABLE_SIZE 1024

WidgetID debug_id_counter = 0;

WidgetFrameTable widget_frame_table_create(LinearArena *arena)
{
    WidgetFrameTable result = {0};
    result.entry_table_size = FRAME_WIDGET_TABLE_SIZE;
    result.entries = la_allocate_array(arena, WidgetList, result.entry_table_size);
    result.arena = la_create(la_allocator(arena), FRAME_WIDGET_TABLE_SIZE * SIZEOF(*result.entries));

    return result;
}

static Widget *widget_frame_table_find(WidgetFrameTable *table, WidgetID id)
{
    ssize index = hash_index(id, table->entry_table_size);

    WidgetList *list = &table->entries[index];

    Widget *result = 0;
    for (Widget *widget = sl_list_head(list); widget; widget = widget->next_in_hash) {
        if (widget->id == id) {
            result = widget;
            break;
        }
    }

    return result;
}

static void widget_frame_table_push(WidgetFrameTable *table, Widget *widget)
{
    ASSERT(!widget_frame_table_find(table, widget->id));

    ssize index = hash_index(widget->id, table->entry_table_size);
    sl_list_push_back_x(&table->entries[index], widget, next_in_hash);
}


void ui_core_set_style(UIState *ui, UIStyle style)
{
    ui->current_style = style;
}

void ui_core_initialize(UIState *ui, UIStyle style, LinearArena *arena)
{
    ui->previous_frame_widgets = widget_frame_table_create(arena);
    ui->current_frame_widgets = widget_frame_table_create(arena);

    ui_core_set_style(ui, style);
}

void ui_core_begin_frame(UIState *ui)
{
    {
        WidgetFrameTable tmp = ui->previous_frame_widgets;
        ui->previous_frame_widgets = ui->current_frame_widgets;
        ui->current_frame_widgets = tmp;

        zero_array(ui->current_frame_widgets.entries, ui->current_frame_widgets.entry_table_size);
        la_reset(&ui->current_frame_widgets.arena);
    }

    debug_id_counter = 0;
    ui->root_widget = 0;
}

static Vector2 get_next_child_position(LayoutKind layout, Vector2 current_pos, f32 padding, Widget *child)
{
    Vector2 result = current_pos;

    if (layout == UI_LAYOUT_VERTICAL) {
        result.y += child->final_size.y + padding;
    } else if (layout == UI_LAYOUT_HORIZONTAL) {
        result.x += child->final_size.x + padding;
    } else {
        ASSERT(0);
    }

    return result;
}

static void calculate_widget_layout(Widget *widget, Vector2 offset, PlatformCode platform_code)
{
    Vector2 final_position =  offset;

    if (widget_has_flag(widget, WIDGET_TEXT)) {
        widget->final_size = platform_code.get_text_dimensions(widget->text.font,
            widget->text.string, widget->text.size);
    } else {
        widget->final_size = widget->preliminary_size;
    }

    widget->final_position = final_position;

    Vector2 next_child_pos = v2_add(final_position, v2(widget->child_padding, widget->child_padding));

    for (Widget *child = sl_list_head(&widget->children); child; child = child->next_sibling) {
        calculate_widget_layout(child, next_child_pos, platform_code);

        next_child_pos = get_next_child_position(widget->layout_kind, next_child_pos, widget->child_padding, child);
    }
}

static void calculate_widget_interactions(Widget *widget, const Input *input)
{
    for (Widget *child = sl_list_head(&widget->children); child; child = child->next_sibling) {
        calculate_widget_interactions(child, input);
    }

    if (widget_has_flag(widget, WIDGET_CLICKABLE)) {
        ASSERT(widget->final_size.x);
        ASSERT(widget->final_size.y);

        Rectangle rect = widget_get_bounding_box(widget);
        if (rect_contains_point(rect, input->mouse_position) && input_is_key_released(input, MOUSE_LEFT)) {
            widget->interaction_state.clicked = true;
        }
    }
}

static LinearArena *get_frame_arena(UIState *ui)
{
    LinearArena *result = &ui->current_frame_widgets.arena;

    return result;
}

static void render_widget(UIState *ui, Widget *widget, RenderBatch *rb, const AssetList *assets, ssize depth)
{
    // TODO: depth isn't needed once a stable sort is implemented for render commands
    // TODO: render differently if clicked
    LinearArena *arena = get_frame_arena(ui);
    RGBA32 color = {0, 1.0f, 0.1f, 0.3f};

    if (widget_has_flag(widget, WIDGET_COLORED)) {
        color = RGBA32_BLUE;
    }

    Rectangle rect = widget_get_bounding_box(widget);
    rb_push_rect(rb, &ui->current_frame_widgets.arena, rect, color, assets->shader2, depth);

    if (widget_has_flag(widget, WIDGET_TEXT)) {
        // TODO: allow changing font size
        rb_push_text(rb, arena, widget->text.string, widget->final_position, RGBA32_WHITE, 12,
            assets->shader, widget->text.font, depth);
    }

    for (Widget *child = sl_list_head(&widget->children); child; child = child->next_sibling) {
        render_widget(ui, child, rb, assets, depth + 1);
    }
}

void ui_core_end_frame(UIState *ui, const struct Input *input, RenderBatch *rb, const AssetList *assets,
    PlatformCode platform_code)
{
    ASSERT(sl_list_is_empty(&ui->container_stack));

    if (ui->root_widget) {
        calculate_widget_layout(ui->root_widget, V2_ZERO, platform_code);
        calculate_widget_interactions(ui->root_widget, input);
        render_widget(ui, ui->root_widget, rb, assets, 0);
    }
}

static Widget *get_top_container(UIState *ui)
{
    WidgetContainer *container = sl_list_head(&ui->container_stack);

    Widget *result = 0;
    if (container) {
        result = container->widget;
    }

    return result;
}

static void widget_add_to_children(Widget *widget, Widget *child)
{
    sl_list_push_back_x(&widget->children, child, next_sibling);
}

void ui_core_push_widget(UIState *ui, Widget *widget)
{
    widget_frame_table_push(&ui->current_frame_widgets, widget);

    if (!ui->root_widget) {
        ui->root_widget = widget;
    }

    Widget *top_container = get_top_container(ui);
    if (top_container) {
        widget_add_to_children(top_container, widget);
    }
}

Widget *ui_core_create_widget(UIState *ui, Vector2 size, WidgetID id)
{
    Widget *widget = la_allocate_item(get_frame_arena(ui), Widget);
    widget->id = id;
    widget->preliminary_size = size;

    ui_core_push_widget(ui, widget);

    return widget;
}

void ui_core_push_as_container(UIState *ui, Widget *widget)
{
    WidgetContainer *container = la_allocate_item(get_frame_arena(ui), WidgetContainer);
    container->widget = widget;

    sl_list_push_front(&ui->container_stack, container);
}

void ui_core_begin_container(UIState *ui, Vector2 size, LayoutKind layout, f32 padding)
{
    Widget *widget = ui_core_create_widget(ui, size, debug_id_counter++);
    widget->layout_kind = layout;
    widget->child_padding = padding;

    ui_core_push_as_container(ui, widget);
}

void ui_core_end_container(UIState *ui)
{
    ASSERT(!sl_list_is_empty(&ui->container_stack));
    sl_list_pop(&ui->container_stack);
}

WidgetInteraction ui_core_get_widget_interaction(UIState *ui, const Widget *widget)
{
    WidgetInteraction result = {0};
    Widget *found = widget_frame_table_find(&ui->previous_frame_widgets, widget->id);

    if (found) {
        result = found->interaction_state;
    }

    return result;
}
