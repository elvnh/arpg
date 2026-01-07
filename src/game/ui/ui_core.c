#include <string.h>

#include "base/hash.h"
#include "base/matrix.h"
#include "base/vector.h"
#include "game/ui/widget.h"
#include "ui_core.h"
#include "asset_table.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "base/sl_list.h"
#include "game/renderer/render_batch.h"
#include "input.h"

#define FRAME_WIDGET_TABLE_SIZE 1024
#define DEFAULT_LAYOUT_AXIS UI_LAYOUT_VERTICAL

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
    if (id == UI_NULL_WIDGET_ID) {
        return 0;
    }

    ssize index = mod_index(id, table->entry_table_size);

    WidgetList *list = &table->entries[index];

    Widget *result = 0;
    for (Widget *widget = list_head(list); widget; widget = widget->next_in_hash) {
        if (widget->id == id) {
            result = widget;
            break;
        }
    }

    return result;
}

static void widget_frame_table_push(WidgetFrameTable *table, Widget *widget)
{
    ASSERT((widget->id == UI_NULL_WIDGET_ID || !widget_frame_table_find(table, widget->id))
        && "Hash collision");

    ssize index = mod_index(widget->id, table->entry_table_size);
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

    ui->hot_widget = UI_NULL_WIDGET_ID;
    ui->root_widget = 0;
}

static b32 widget_is_hot(UIState *ui, Widget *widget)
{
    b32 result = (widget->id != UI_NULL_WIDGET_ID) && (ui->hot_widget == widget->id);
    return result;
}

static b32 widget_is_active(UIState *ui, Widget *widget)
{
    b32 result = (widget->id != UI_NULL_WIDGET_ID) && (ui->active_widget == widget->id);
    return result;
}

typedef enum {
    TRAVERSAL_ORDER_PREORDER,
    TRAVERSAL_ORDER_POSTORDER,
} TraversalOrder;

static TraversalOrder get_layout_traversal_order(UISizeKind size_kind)
{
    switch (size_kind) {
        case UI_SIZE_KIND_ABSOLUTE:          return TRAVERSAL_ORDER_PREORDER;
        case UI_SIZE_KIND_PERCENT_OF_PARENT: return TRAVERSAL_ORDER_PREORDER;
        case UI_SIZE_KIND_SUM_OF_CHILDREN:   return TRAVERSAL_ORDER_POSTORDER;
        INVALID_DEFAULT_CASE;
    }

    ASSERT(0);
    return 0;
}

static void calculate_widget_layout(Widget *widget, Vector2 offset, PlatformCode platform_code, Widget *parent);

static void calculate_layout_of_children(Widget *widget, PlatformCode platform_code)
{
    Vector2 next_child_pos = v2_add(widget->final_position, v2(widget->child_padding, widget->child_padding));
    f32 current_row_size = 0.0f;

    for (Widget *child = list_head(&widget->children); child; child = child->next_sibling) {
        calculate_widget_layout(child, next_child_pos, platform_code, widget);
        current_row_size = MAX(current_row_size, child->final_size.y);

        Widget *next = child->next_sibling;
        if (next) {
            if (next->layout_direction == UI_LAYOUT_HORIZONTAL) {
                next_child_pos.x += child->final_size.x + widget->child_padding;
            } else {
                next_child_pos.x = widget->final_position.x + widget->child_padding;
                next_child_pos.y += current_row_size + widget->child_padding;
                current_row_size = 0.0f;
            }
        }
    }
}

static void calculate_widget_layout_on_axis(Widget *widget, Vector2 offset, PlatformCode platform_code,
    Widget *parent, Axis axis)
{
    UISizeKind size_kind = widget->semantic_size[axis].kind;
    TraversalOrder order = get_layout_traversal_order(size_kind);

    if (order == TRAVERSAL_ORDER_POSTORDER) {
        calculate_layout_of_children(widget, platform_code);
    }

    switch (size_kind) {
        case UI_SIZE_KIND_ABSOLUTE: {
            *v2_index(&widget->final_size, axis) = widget->semantic_size[axis].value;
        } break;

        case UI_SIZE_KIND_SUM_OF_CHILDREN: {
            f32 max_bounds = 0.0f;

            for (Widget *child = list_head(&widget->children); child; child = child->next_sibling) {
                f32 child_bounds = *v2_index(&child->final_position, axis) + *v2_index(&child->final_size, axis)
                    - *v2_index(&offset, axis);

                max_bounds = MAX(max_bounds, child_bounds);
            }

            *v2_index(&widget->final_size, axis) = max_bounds + widget->child_padding;
        } break;

        case UI_SIZE_KIND_PERCENT_OF_PARENT: {
            ASSERT(parent);
            ASSERT(f32_in_range(widget->semantic_size[axis].value, 0.0f, 1.0f, 0.0f));

            ASSERT(parent->semantic_size[axis].kind != UI_SIZE_KIND_SUM_OF_CHILDREN);

            f32 parent_size_on_axis = *v2_index(&parent->final_size, axis) - parent->child_padding * 2;

            *v2_index(&widget->final_size, axis) = widget->semantic_size[axis].value * parent_size_on_axis;
        } break;

        INVALID_DEFAULT_CASE;
    }

    if (order == TRAVERSAL_ORDER_PREORDER) {
        calculate_layout_of_children(widget, platform_code);
    }

    if (widget_has_flag(widget, WIDGET_TEXT)) {
        f32 baseline = platform_code.get_font_baseline_offset(widget->text.font, widget->text.size);
        Vector2 text_dims = platform_code.get_text_dimensions(widget->text.font,
            widget->text.string, widget->text.size);

        widget->final_size = text_dims;
        widget->text.baseline_y_offset = baseline;
    }

}

static Rectangle widget_get_clipped_bounding_box(Widget *widget, Rectangle parent_bounds)
{
    Rectangle unclipped = widget_get_bounding_box(widget);
    Rectangle result  = rect_overlap_area(unclipped, parent_bounds);

    return result;
}

static void calculate_widget_layout(Widget *widget, Vector2 offset, PlatformCode platform_code, Widget *parent)
{
    widget->final_position = v2_add(offset, widget->offset_from_parent);

    for (Axis axis = 0; axis < AXIS_COUNT; ++axis) {
        calculate_widget_layout_on_axis(widget, offset, platform_code, parent, axis);
    }

}

typedef enum {
    CALC_INTERACTION_STOP,
    CALC_INTERACTION_KEEP_GOING,
} CalculateInteractionResult;

static CalculateInteractionResult
calculate_widget_interactions(UIState *ui, Widget *widget, const FrameData *frame_data,
    Rectangle parent_bounds, YDirection y_dir, UIInteraction *interactions)
{
    Rectangle clipped_bounds = widget_get_clipped_bounding_box(widget, parent_bounds);

    for (Widget *child = list_head(&widget->children); child; child = child->next_sibling) {
        CalculateInteractionResult child_result =
            calculate_widget_interactions(ui, child, frame_data, clipped_bounds, y_dir, interactions);

        // If child widget is being interacted with, don't consider input further up the tree
        if (child_result == CALC_INTERACTION_STOP) {
            return CALC_INTERACTION_STOP;
        }
    }

    if (widget->id == UI_NULL_WIDGET_ID) {
        return CALC_INTERACTION_KEEP_GOING;
    }

    Vector2 mouse_pos = input_get_mouse_pos(&frame_data->input, y_dir, frame_data->window_size);
    Vector2 mouse_click_pos = input_get_mouse_click_pos(&frame_data->input, y_dir, frame_data->window_size);
    b32 mouse_inside = rect_contains_point(clipped_bounds, mouse_pos);
    b32 mouse_clicked = input_is_key_down(&frame_data->input, MOUSE_LEFT);
    b32 mouse_released = input_is_key_released(&frame_data->input, MOUSE_LEFT);
    b32 click_began_inside = rect_contains_point(clipped_bounds, mouse_click_pos);
    b32 clicked_inside = mouse_clicked && click_began_inside;

    if (mouse_inside) {
        ui->hot_widget = widget->id;
    }

    if (widget_is_hot(ui, widget)) {
	if (mouse_inside) {
	    widget->interaction_state.hovered = true;
	}

        if (widget_has_flag(widget, WIDGET_CLICKABLE) && clicked_inside) {
            ui->active_widget = widget->id;
        }
    }

    if (widget_is_active(ui, widget)) {
        if (!mouse_inside && !widget_has_flag(widget, WIDGET_STAY_ACTIVE)) {
            ui->active_widget = UI_NULL_WIDGET_ID;
        } else if (mouse_released) {
            widget->interaction_state.clicked = true;

            if (!widget_has_flag(widget, WIDGET_STAY_ACTIVE)) {
                ui->active_widget = UI_NULL_WIDGET_ID;
            }
        }

        // TODO: proper text input
        if (widget_has_flag(widget, WIDGET_TEXT_INPUT)) {
            if (input_is_key_pressed(&frame_data->input, KEY_A)) {
                if (str_builder_has_capacity_for(widget->text_input_buffer, str_lit("A"))) {
                    str_builder_append(widget->text_input_buffer, str_lit("A"));
                }
            }
        }
    }

    if (mouse_inside) {
	interactions->was_hovered = true;

	if (mouse_clicked || mouse_released) {
	    interactions->received_mouse_input = true;
	}
    }

    if (click_began_inside) {
	interactions->click_began_inside_ui = true;
    }

    if (widget_is_hot(ui, widget) || widget_is_active(ui, widget)) {
        return CALC_INTERACTION_STOP;
    }

    return CALC_INTERACTION_KEEP_GOING;
}

static LinearArena *get_frame_arena(UIState *ui)
{
    LinearArena *result = &ui->current_frame_widgets.arena;

    return result;
}

static void render_widget(UIState *ui, Widget *widget, RenderBatch *rb, ssize depth, Rectangle parent_bounds)
{
    // TODO: depth isn't needed once a stable sort is implemented for render commands
#if 1
    Rectangle widget_rect = widget_get_clipped_bounding_box(widget, parent_bounds);

    if (!widget_has_flag(widget, WIDGET_HIDDEN)) {
        LinearArena *arena = get_frame_arena(ui);

        if (widget_has_flag(widget, WIDGET_COLORED)) {
            RGBA32 color = widget->color;

            if (widget_is_hot(ui, widget) && widget_has_flag(widget, WIDGET_HOT_COLOR)) {
                color = RGBA32_GREEN;
            }

            if (widget_is_active(ui, widget) && widget_has_flag(widget, WIDGET_ACTIVE_COLOR)) {
                color = RGBA32_RED;
            }

            rb_push_rect(rb, &ui->current_frame_widgets.arena, widget_rect, color,
		get_asset_table()->shape_shader, depth);
        }

        if (widget_has_flag(widget, WIDGET_TEXT)) {
            Vector2 text_position = widget->final_position;

	    if (rb->y_direction == Y_IS_DOWN) {
		// TODO: this offset doesn't seem quite right
		text_position = v2_add(text_position, v2(0.0f, widget->text.baseline_y_offset));
	    }

            rb_push_clipped_text(rb, arena, widget->text.string, text_position,
		parent_bounds, widget->color, widget->text.size,
                get_asset_table()->texture_shader, widget->text.font, depth);
        }
    }
#else
    if (true || !widget_has_flag(widget, WIDGET_HIDDEN)) {
        LinearArena *arena = get_frame_arena(ui);

        if (true || widget_has_flag(widget, WIDGET_COLORED)) {
            Rectangle widget_rect = widget_get_bounding_box(widget);
            RGBA32 color = widget->color;

            if (widget_is_hot(ui, widget)) {
                color = RGBA32_GREEN;
            }

            if (widget_is_active(ui, widget)) {
                color = RGBA32_RED;
            }

            if (!widget_has_flag(widget, WIDGET_COLORED) || widget_has_flag(widget, WIDGET_HIDDEN)) {
                color = (RGBA32){0, 1, 0, 0.5f};
            }

            rb_push_rect(rb, &ui->current_frame_widgets.arena, widget_rect, color, assets->shape_shader, depth);
        }

        if (widget_has_flag(widget, WIDGET_TEXT)) {
            Vector2 text_position = v2_add(widget->final_position, v2(0.0f, widget->text.baseline_y_offset));

            // TODO: newlines don't work properly
            rb_push_text(rb, arena, widget->text.string, text_position, widget->color, widget->text.size,
                assets->texture_shader, widget->text.font, depth);
        }
    }
#endif

    for (Widget *child = list_head(&widget->children); child; child = child->next_sibling) {
        render_widget(ui, child, rb, depth + 1, widget_rect);
    }
}

UIInteraction ui_core_end_frame(UIState *ui, const FrameData *frame_data, RenderBatch *rb,
    PlatformCode platform_code)
{
    ASSERT(ui->current_layout_axis == DEFAULT_LAYOUT_AXIS);
    ASSERT(list_is_empty(&ui->container_stack));

    UIInteraction result = {0};

    Rectangle window_rect = {{0, 0}, v2i_to_v2(frame_data->window_size)};

    if (ui->root_widget) {
        calculate_widget_layout(ui->root_widget, V2_ZERO, platform_code, 0);
        calculate_widget_interactions(ui, ui->root_widget, frame_data, window_rect, rb->y_direction, &result);

        render_widget(ui, ui->root_widget, rb, 0, window_rect);
    }

    return result;
}

Widget *ui_core_get_top_container(UIState *ui)
{
    WidgetContainer *container = list_head(&ui->container_stack);

    Widget *result = 0;
    if (container) {
        result = container->widget;
    }

    return result;
}

static void ui_core_push_widget(UIState *ui, Widget *widget)
{
    widget_frame_table_push(&ui->current_frame_widgets, widget);

    if (!ui->root_widget) {
        ui->root_widget = widget;
    }

    Widget *top_container = ui_core_get_top_container(ui);

    widget->layout_direction = ui->current_layout_axis;
    ui->current_layout_axis = DEFAULT_LAYOUT_AXIS;

    if (top_container) {
        widget_add_to_children(top_container, widget);
    }
}

Widget *ui_core_create_widget(UIState *ui, Vector2 size, WidgetID id)
{
    Widget *widget = la_allocate_item(get_frame_arena(ui), Widget);
    widget->id = id;

    widget->semantic_size[AXIS_HORIZONTAL] = (WidgetSize){ .value = size.x };
    widget->semantic_size[AXIS_VERTICAL] = (WidgetSize){ .value = size.y };

    ui_core_push_widget(ui, widget);

    return widget;
}

Widget *ui_core_colored_box(UIState *ui, Vector2 size, RGBA32 color, WidgetID id)
{
    Widget *widget = ui_core_create_widget(ui, size, id);
    widget_add_flag(widget, WIDGET_COLORED);
    widget->color = color;

    return widget;
}

void ui_core_push_container(UIState *ui, Widget *widget)
{
    WidgetContainer *container = la_allocate_item(get_frame_arena(ui), WidgetContainer);
    container->widget = widget;

    sl_list_push_front(&ui->container_stack, container);
}

void ui_core_pop_container(UIState *ui)
{
    ASSERT(!list_is_empty(&ui->container_stack));
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

void ui_core_same_line(UIState *ui)
{
    ui->current_layout_axis = UI_LAYOUT_HORIZONTAL;
}

WidgetID ui_core_hash_string(String text)
{
    // TODO: hash but don't make visible characters after ##
    WidgetID result = hash_string(text);
    ASSERT(result != UI_NULL_WIDGET_ID);

    return result;
}
