#ifndef DEBUG_H
#define DEBUG_H

typedef struct DebugState {
    b32 debug_menu_active;
    b32 quad_tree_overlay;
    b32 render_colliders;
    b32 render_origin;
    b32 render_entity_bounds;
    f32 average_fps;

    EntityID hovered_entity;
} DebugState;


#endif //DEBUG_H
