#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#include "camera.h"
#include "ui/ui_core.h"

struct Game;
struct GameMemory;
struct FrameData;
struct QuadTreeNode;
struct RenderBatch;
struct LinearArena;

typedef struct DebugState {
    UIState debug_ui;

    b32 debug_menu_active;
    b32 quad_tree_overlay;
    b32 render_colliders;
    b32 render_origin;
    b32 render_entity_bounds;
    b32 render_entity_velocity;
    b32 render_edge_list;

    f32 average_fps;
    f32 timestep_modifier;

    // TODO: asset memory usage
    ssize scratch_arena_memory_usage;
    ssize permanent_arena_memory_usage;
    ssize world_arena_memory_usage;

    Camera debug_camera;
    b32 debug_camera_active;

    b32 render_camera_bounds;
} DebugState;

static inline void print_v2(Vector2 v)
{
    printf("(%.2f, %.2f)\n", (f64)v.x, (f64)v.y);
}

void debug_update(struct Game *game, const struct FrameData *frame_data, LinearArena *frame_arena);
void debug_ui(UIState *ui, struct Game *game, struct LinearArena *scratch, const struct FrameData *frame_data);
void render_quad_tree(struct QuadTreeNode *tree, struct RenderBatch *rb, LinearArena *arena,
    ssize depth);

#endif //DEBUG_H
