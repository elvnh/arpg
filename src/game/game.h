#ifndef GAME_H
#define GAME_H

#include "base/matrix.h"
#include "renderer/render_batch.h"
#include "camera.h"
#include "input.h"

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 size;
    RGBA32 color;
} Entity;

typedef struct {
    Entity entities[32];
    s32  entity_count;
    Camera camera;
} GameWorld;

typedef struct {
    LinearArena frame_arena;
    GameWorld world;
} GameState;

typedef struct {
    f32 dt;
    const Input *input;
    s32 window_width;
    s32 window_height;
} FrameData;

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets,
    FrameData frame_data);


#endif //GAME_H
