#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#include "ui/ui_core.h"

typedef struct DebugState {
    UIState debug_ui;

    b32 debug_menu_active;
    b32 quad_tree_overlay;
    b32 render_colliders;
    b32 render_origin;
    b32 render_entity_bounds;
    f32 average_fps;
    f32 timestep_modifier;

    EntityID hovered_entity;
} DebugState;

static inline void print_v2(Vector2 v)
{
    printf("(%.2f, %.2f)\n", (f64)v.x, (f64)v.y);
}

#endif //DEBUG_H
