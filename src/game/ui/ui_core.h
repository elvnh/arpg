#ifndef UI_H
#define UI_H

#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/vector.h"
#include "base/rectangle.h"
#include "asset.h"
#include "platform.h"
#include "widget.h"

/*
  TODO:
  - Layout stack, keep track of next position etc
 */

#define UI_NULL_WIDGET_ID 0

extern WidgetID debug_id_counter;

struct Input;
struct RenderBatch;
struct AssetList;

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

    UILayoutKind current_layout_axis;
    UIStyle current_style; // TODO: style stack
    Widget *root_widget; // TODO: push container that contains entire viewport on beginning of each frame
} UIState;

// TODO: remove ui_core_ prefixes?

void ui_core_initialize(UIState *ui, UIStyle style, LinearArena *arena);
void ui_core_begin_frame(UIState *ui);
void ui_core_end_frame(UIState *ui, const struct Input *input, struct RenderBatch *rb, const struct AssetList *assets,
    PlatformCode platform_code);
void ui_core_set_style(UIState *ui, UIStyle style);
void ui_core_begin_container(UIState *ui, Vector2 size, UISizeKind size_kind, f32 padding);
void ui_core_end_container(UIState *ui);
void ui_core_push_as_container(UIState *ui, Widget *widget);
Widget *ui_core_create_widget(UIState *ui, Vector2 size, WidgetID id);
Widget *ui_core_colored_box(UIState *ui, Vector2 size, RGBA32 color, WidgetID id);
WidgetInteraction ui_core_get_widget_interaction(UIState *ui, const Widget *widget);
void ui_core_same_line(UIState *ui);
WidgetID ui_core_hash_string(String text);

#endif //UI_H
