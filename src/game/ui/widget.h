#ifndef WIDGET_H
#define WIDGET_H

#include "base/vector.h"
#include "base/sl_list.h"
#include "base/string8.h"
#include "base/rectangle.h"
#include "asset.h"

typedef u64 WidgetID;

typedef enum {
    WIDGET_CLICKABLE = (1u << 0),
    WIDGET_COLORED   = (1u << 1),
    WIDGET_TEXT      = (1u << 2),
    WIDGET_HIDDEN    = (1u << 3),
} WidgetFlag;

typedef enum {
    UI_LAYOUT_VERTICAL,
    UI_LAYOUT_HORIZONTAL,
} UILayoutKind;

typedef enum {
    UI_SIZE_KIND_ABSOLUTE,
    UI_SIZE_KIND_SUM_OF_CHILDREN,
    UI_SIZE_KIND_PERCENT_OF_PARENT,
} UISizeKind;

typedef struct {
    b32 clicked;
} WidgetInteraction;

typedef struct {
    struct Widget *head;
    struct Widget *tail;
} WidgetList;

typedef struct Widget {
    WidgetID id;
    WidgetInteraction interaction_state; // TODO: should this be in separate struct?
    WidgetFlag flags;

    UISizeKind size_kind;
    Vector2    preliminary_size;
    Vector2    offset_from_parent;

    // NOTE: these are always absolute
    Vector2    final_size;
    Vector2    final_position;

    UILayoutKind     parent_layout_kind;

    f32              child_padding;
    WidgetList       children;
    struct Widget   *next_in_hash;
    struct Widget   *next_sibling;

    /* Flag specific variables */
    struct {
        String string;
        FontHandle font;
        s32 size;
    } text;

    RGBA32 color;
} Widget;

static inline b32 widget_has_flag(const Widget *widget, WidgetFlag flag)
{
    b32 result = (widget->flags & flag) != 0;

    return result;
}

static inline void widget_add_flag(Widget *widget, WidgetFlag flag)
{
    widget->flags |= flag;
}

static inline Rectangle widget_get_bounding_box(Widget *widget)
{
    Rectangle result = {widget->final_position, widget->final_size};

    return result;
}

static inline void widget_add_to_children(Widget *widget, Widget *child)
{
    sl_list_push_back_x(&widget->children, child, next_sibling);
}

#endif //WIDGET_H
