#ifndef UI_H
#define UI_H

#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/vector.h"
#include "base/rectangle.h"
#include "asset.h"

/*
  TODO:
  - Layout stack, keep track of next position etc
 */

typedef u32 WidgetID;

struct Input;
struct RenderBatch;
struct AssetList;

typedef struct {
    b32 clicked;
} WidgetInteraction;

typedef struct {
    struct Widget *head;
    struct Widget *tail;
} WidgetList;

typedef enum {
    WIDGET_CLICKABLE = (1u << 0),
    WIDGET_COLORED   = (1u << 1),
    WIDGET_TEXT      = (1u << 2),
} WidgetFlag;

typedef enum {
    UI_LAYOUT_VERTICAL,
    UI_LAYOUT_HORIZONTAL,
} LayoutKind;

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
    } text;
} Widget;

typedef struct {
    WidgetList *entries;
    ssize entry_table_size;
    LinearArena arena;
} WidgetFrameTable;

typedef struct WidgetContainer {
    Widget *widget;
    struct WidgetContainer *next;
} WidgetContainer;

typedef struct {
    WidgetContainer *head;
    WidgetContainer *tail;
} WidgetContainerStack;

typedef struct {
    FontHandle font;
    // TODO: color theme
} UIStyle;

typedef struct {
    WidgetFrameTable previous_frame_widgets;
    WidgetFrameTable current_frame_widgets;
    WidgetContainerStack container_stack;

    UIStyle current_style; // TODO: style stack
    Widget *root_widget; // TODO: push container that contains entire viewport on beginning of each frame
} UIState;

void ui_initialize(UIState *ui, UIStyle style, LinearArena *arena);
void ui_begin_frame(UIState *ui);
void ui_set_style(UIState *ui, UIStyle style);
void ui_end_frame(UIState *ui, const struct Input *input, struct RenderBatch *rb, const struct AssetList *assets);
void ui_begin_container(UIState *ui, Vector2 size, LayoutKind layout, f32 padding);
void ui_end_container(UIState *ui);

// TODO: move these to separate file
WidgetInteraction ui_button(UIState *ui, String text);
Widget *ui_label(UIState *ui, String text);

#endif //UI_H
