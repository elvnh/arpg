#ifndef UI_H
#define UI_H

#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/string8.h"
#include "base/vector.h"
#include "platform/input_event.h"
#include "platform/platform.h"
#include "widget.h"

/*
  TODO:
  - Centering?
  - Scrolling lists
  - Fix the issues with having to provide layers due to
    render command sort not being stable
 */

#define UI_NULL_WIDGET_ID 0
#define UI_STYLE_TRANSPARENT (UIStyle) {0}

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

typedef struct UIStyle {
    FontHandle font;

    RGBA32 text_color;
    RGBA32 background_color;
    RGBA32 background_shadow_color;
    RGBA32 context_menu_color;
    RGBA32 accent_color;
    RGBA32 active_color;
    RGBA32 hot_color;

    struct UIStyle *next_style_in_stack;
    b32 pop_after_one_use;
} UIStyle;

typedef struct {
    // TODO: maybe received_mouse_input and click_began_inside_ui should be same field?
    b32 received_mouse_input;
    b32 click_began_inside_ui;
    b32 was_hovered;
} UIInteraction;

typedef struct UIState {
    WidgetFrameTable previous_frame_widgets;
    WidgetFrameTable current_frame_widgets;
    WidgetContainerStack container_stack;
    //WidgetContainerStack floating_container_stack;

    WidgetID hot_widget;
    WidgetID active_widget;

    UILayoutKind current_layout_axis;
    UIAlignment current_alignment[AXIS_COUNT];

    UIStyle default_style;
    UIStyle *style_stack;

    // TODO: push container that contains entire viewport on beginning of each frame
    Widget *root_widget;

    WidgetList floating_widgets;

    b32 frame_started;
} UIState;

// TODO: remove ui_core_ prefixes?

// TODO: floatign shouldn't be a bool parameter

void ui_initialize(UIState *ui, UIStyle default_style, LinearArena *arena);
void ui_begin_frame(UIState *ui);
UIInteraction ui_end_layout(UIState *ui, struct FrameInput *frame_data, YDirection y_dir,
    PlatformCode platform_code);
void ui_render(UIState *ui, FrameInput *frame_input, struct RenderBatch *rb);
void ui_push_container(UIState *ui, Widget *widget);
void ui_pop_container(UIState *ui);
Widget *ui_get_top_container(UIState *ui);
Widget *ui_create_widget(UIState *ui, Vector2 size, WidgetID id, b32 floating);
Widget *ui_colored_box(UIState *ui, Vector2 size, RGBA32 color, WidgetID id, b32 floating);
WidgetInteraction ui_get_widget_interaction(UIState *ui, const Widget *widget);
void ui_same_line(UIState *ui);
WidgetID ui_create_id(String text);
void ui_set_next_alignment(UIState *ui, UIAlignment alignment, Axis axis);

UIStyle ui_default_style(UIState *ui);
UIStyle ui_get_current_style(UIState *ui);
void ui_set_next_style(UIState *ui, UIStyle style);
void ui_push_style(UIState *ui, UIStyle style);
void ui_pop_style(UIState *ui);

#endif //UI_H
