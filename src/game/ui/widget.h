#ifndef WIDGET_H
#define WIDGET_H

#include "base/typedefs.h"

typedef u32 WidgetID;

typedef enum {
    WIDGET_CLICKABLE = (1u << 0),
    WIDGET_COLORED   = (1u << 1),
    WIDGET_TEXT      = (1u << 2),
} WidgetFlag;

typedef enum {
    UI_LAYOUT_VERTICAL,
    UI_LAYOUT_HORIZONTAL,
} LayoutKind;

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

    Vector2   offset_from_parent;
    Vector2   preliminary_size;
    Vector2   final_position;
    Vector2   final_size;

    LayoutKind     layout_kind;
    f32            child_padding;
    WidgetList     children;
    struct Widget *next_in_hash;
    struct Widget *next_sibling;

    struct {
        String string;
        FontHandle font;
        s32 size;
    } text;
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

#endif //WIDGET_H
