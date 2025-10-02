#ifndef UI_H
#define UI_H

#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/vector.h"
#include "base/rectangle.h"

/*
  TODO:
  - Layout kinds
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
    WIDGET_COLORED = (1u << 1),
} WidgetFlag;

typedef enum {
    UI_LAYOUT_VERTICAL,
} LayoutKind;

typedef struct Widget {
    WidgetID id;
    WidgetInteraction interaction_state; // TODO: should this be in separate struct?
    WidgetFlag flags;

    Vector2   offset_from_parent;
    Vector2   preliminary_size; // TODO: can be changed, create final_size

    Vector2 final_position;
    Vector2 final_size;

    LayoutKind layout_kind;
    WidgetList    children;
    struct Widget *next; // TODO: rename
} Widget;

typedef struct {
    WidgetList *entries;
    ssize entry_table_size;
    LinearArena arena;
} WidgetFrameTable;

typedef struct {
    WidgetFrameTable previous_frame_widgets;
    WidgetFrameTable current_frame_widgets;
    WidgetList container_stack;

    Widget *root_widget; // TODO: push container that contains entire viewport on beginning of each frame
} UIState;

void ui_initialize(UIState *ui, LinearArena *arena);
void ui_begin_frame(UIState *ui);
void ui_end_frame(UIState *ui, const struct Input *input, struct RenderBatch *rb, const struct AssetList *assets);
void ui_begin_container(UIState *ui, Vector2 offset, Vector2 size, LayoutKind layout);
void ui_end_container(UIState *ui);

// TODO: move these to separate file
WidgetInteraction ui_button(UIState *ui, String text, Vector2 position);

#endif //UI_H
