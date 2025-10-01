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
} WidgetFlag;

typedef struct Widget {
    WidgetID id;
    WidgetInteraction interaction_state; // TODO: should this be in separate struct?
    WidgetFlag flags;

    Vector2   offset;
    Vector2   size; // TODO: can be changed, create final_size

    Vector2 final_position;

    WidgetList    children;
    struct Widget *next; // TODO: rename
} Widget;

typedef struct {
    WidgetList *entries;
    ssize entry_table_size;
    LinearArena arena;
} FrameWidgetTable;

typedef struct {
    FrameWidgetTable previous_frame_widgets;
    FrameWidgetTable current_frame_widgets;

    Widget *root_widget; // TODO: push container that contains entire viewport on beginning of each frame
} UIState;

void ui_initialize(UIState *ui, LinearArena *arena);
void ui_begin_frame(UIState *ui);
void ui_end_frame(UIState *ui, const struct Input *input, struct RenderBatch *rb, const struct AssetList *assets);

// TODO: move these to separate file
WidgetInteraction ui_button(UIState *ui, String text, Vector2 position);

#endif //UI_H
