#include <string.h>

#include "game/ui/widget.h"
#include "ui_core.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "base/sl_list.h"
#include "game/renderer/render_batch.h"
#include "input.h"

#define FRAME_WIDGET_TABLE_SIZE 1024
#define DEFAULT_LAYOUT_AXIS UI_LAYOUT_VERTICAL

// TODO: get rid of this
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
    // TODO: these calculations are kind of hacky
    Vector2 next_child_pos = v2_add(widget->final_position, v2(widget->child_padding, widget->child_padding));
    f32 current_row_size = 0.0f;

    for (Widget *child = sl_list_head(&widget->children); child; child = child->next_sibling) {
        calculate_widget_layout(child, next_child_pos, platform_code, widget);
        current_row_size = MAX(current_row_size, child->final_size.y);

        Widget *next = child->next_sibling;
        if (next) {
            if (next->parent_layout_kind == UI_LAYOUT_HORIZONTAL) {
                next_child_pos.x += child->final_size.x + widget->child_padding;
            } else {
                next_child_pos.x = widget->final_position.x + widget->child_padding;
                next_child_pos.y += current_row_size + widget->child_padding;
                current_row_size = 0.0f;
            }
        }
    }
}

static void calculate_widget_layout(Widget *widget, Vector2 offset, PlatformCode platform_code, Widget *parent)
{
    widget->final_position = offset;

    TraversalOrder order = get_layout_traversal_order(widget->size_kind);

    if (order == TRAVERSAL_ORDER_POSTORDER) {
        calculate_layout_of_children(widget, platform_code);
    }

    switch (widget->size_kind) {
        case UI_SIZE_KIND_ABSOLUTE: {
            widget->final_size = widget->preliminary_size;
        } break;
        case UI_SIZE_KIND_SUM_OF_CHILDREN: {
            f32 max_x = 0.0f;
            f32 max_y = 0.0f;

            for (Widget *child = sl_list_head(&widget->children); child; child = child->next_sibling) {
                max_x = MAX(max_x, child->final_position.x + child->final_size.x - offset.x);
                max_y = MAX(max_y, child->final_position.y + child->final_size.y - offset.y);
            }

            widget->final_size = v2(max_x + widget->child_padding, max_y + widget->child_padding);
        } break;

        case UI_SIZE_KIND_PERCENT_OF_PARENT: {
            ASSERT(parent);
            ASSERT(parent->size_kind != UI_SIZE_KIND_SUM_OF_CHILDREN);
            ASSERT(f32_in_range(widget->preliminary_size.x, 0.0f, 1.0f, 0.0f));
            ASSERT(f32_in_range(widget->preliminary_size.y, 0.0f, 1.0f, 0.0f));

            widget->final_size = v2(
                (parent->final_size.x - parent->child_padding * 2) * widget->preliminary_size.x,
                (parent->final_size.y - parent->child_padding * 2) * widget->preliminary_size.y
            );
        } break;

        INVALID_DEFAULT_CASE;
    }



    if (order == TRAVERSAL_ORDER_PREORDER) {
        calculate_layout_of_children(widget, platform_code);
    }

    if (widget_has_flag(widget, WIDGET_TEXT)) {
        widget->final_size = platform_code.get_text_dimensions(widget->text.font,
            widget->text.string, widget->text.size);
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
    if (!widget_has_flag(widget, WIDGET_HIDDEN)) {
        LinearArena *arena = get_frame_arena(ui);
        RGBA32 color = {0, 1.0f, 0.1f, 0.3f};

        if (widget_has_flag(widget, WIDGET_COLORED)) {
            color = widget->color;
        }

        Rectangle rect = widget_get_bounding_box(widget);
        rb_push_rect(rb, &ui->current_frame_widgets.arena, rect, color, assets->shader2, depth);

        if (widget_has_flag(widget, WIDGET_TEXT)) {
            // TODO: allow changing font size
            rb_push_text(rb, arena, widget->text.string, widget->final_position, RGBA32_WHITE, 12,
                assets->shader, widget->text.font, depth);
        }
    }

    for (Widget *child = sl_list_head(&widget->children); child; child = child->next_sibling) {
        render_widget(ui, child, rb, assets, depth + 1);
    }
}

void ui_core_end_frame(UIState *ui, const struct Input *input, RenderBatch *rb, const AssetList *assets,
    PlatformCode platform_code)
{
    ASSERT(ui->current_layout_axis == DEFAULT_LAYOUT_AXIS);
    ASSERT(sl_list_is_empty(&ui->container_stack));

    if (ui->root_widget) {
        calculate_widget_layout(ui->root_widget, V2_ZERO, platform_code, 0);
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

static void ui_core_push_widget(UIState *ui, Widget *widget)
{
    widget_frame_table_push(&ui->current_frame_widgets, widget);

    if (!ui->root_widget) {
        ui->root_widget = widget;
    }

    Widget *top_container = get_top_container(ui);

    widget->parent_layout_kind = ui->current_layout_axis;
    ui->current_layout_axis = DEFAULT_LAYOUT_AXIS;

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

Widget *ui_core_colored_box(UIState *ui, Vector2 size, RGBA32 color, WidgetID id)
{
    Widget *widget = ui_core_create_widget(ui, size, id);
    widget_add_flag(widget, WIDGET_COLORED);
    widget->color = color;

    return widget;
}

void ui_core_push_as_container(UIState *ui, Widget *widget)
{
    WidgetContainer *container = la_allocate_item(get_frame_arena(ui), WidgetContainer);
    container->widget = widget;
    sl_list_push_front(&ui->container_stack, container);
}

// TODO: not needed, implement in builder code that creates windows, groups etc
void ui_core_begin_container(UIState *ui, Vector2 size, UISizeKind size_kind, f32 padding)
{
    Widget *widget = ui_core_create_widget(ui, size, debug_id_counter++);
    widget->child_padding = padding;
    widget->size_kind = size_kind;

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

void ui_core_same_line(UIState *ui)
{
    ui->current_layout_axis = UI_LAYOUT_HORIZONTAL;
}
