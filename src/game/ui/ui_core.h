#ifndef UI_H
#define UI_H

#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/vector.h"
#include "base/rectangle.h"
#include "platform/platform.h"
#include "platform/input.h"
#include "widget.h"

/*
  TODO:
  - Centering?
  - Scrolling lists
 */

#define UI_NULL_WIDGET_ID 0

struct Input;
struct RenderBatch;

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
    // TODO: maybe received_mouse_input and click_began_inside_ui should be same field?
    b32 received_mouse_input;
    b32 click_began_inside_ui;
    b32 was_hovered;
} UIInteraction;

typedef struct {
    WidgetFrameTable previous_frame_widgets;
    WidgetFrameTable current_frame_widgets;
    WidgetContainerStack container_stack;
    //WidgetContainerStack floating_container_stack;

    WidgetID hot_widget;
    WidgetID active_widget;

    UILayoutKind current_layout_axis;
    UIStyle current_style; // TODO: style stack
    Widget *root_widget; // TODO: push container that contains entire viewport on beginning of each frame

    WidgetList floating_widgets;
} UIState;

// TODO: remove ui_core_ prefixes?

// TODO: floatign shouldn't be a bool parameter

void ui_core_initialize(UIState *ui, UIStyle style, LinearArena *arena);
void ui_core_begin_frame(UIState *ui);
UIInteraction ui_core_end_layout(UIState *ui, const FrameData *frame_data, YDirection y_dir,
    PlatformCode platform_code);
void ui_core_render(UIState *ui, const FrameData *frame_data, struct RenderBatch *rb);
void ui_core_set_style(UIState *ui, UIStyle style);
void ui_core_push_container(UIState *ui, Widget *widget);
void ui_core_pop_container(UIState *ui);
Widget *ui_core_get_top_container(UIState *ui);
Widget *ui_core_create_widget(UIState *ui, Vector2 size, WidgetID id, b32 floating);
Widget *ui_core_colored_box(UIState *ui, Vector2 size, RGBA32 color, WidgetID id, b32 floating);
WidgetInteraction ui_core_get_widget_interaction(UIState *ui, const Widget *widget);
void ui_core_same_line(UIState *ui);
WidgetID ui_core_hash_string(String text);

#endif //UI_H
