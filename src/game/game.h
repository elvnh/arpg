#ifndef GAME_H
#define GAME_H

#include "renderer/render_batch.h"
#include "input.h"

typedef struct {
    f32 radius;
    b32 is_colliding;
} CircleCollider;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 size;
    RGBA32 color;
} Entity;

typedef struct {
    Entity entities[32];
    s32  entity_count;
} GameWorld;

typedef struct {
    LinearArena frame_arena;
    GameWorld world;
} GameState;

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets,
    const Input *input);

#endif //GAME_H
