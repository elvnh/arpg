#ifndef GAME_H
#define GAME_H

#include "base/matrix.h"
#include "renderer/render_batch.h"
#include "game_world.h"
#include "input.h"

typedef struct {
    GameWorld world;
} GameState;

typedef struct {
    f32 dt;
    const Input *input;
    s32 window_width;
    s32 window_height;
} FrameData;

void game_update_and_render(GameState *game_state, RenderBatchList *rbs, const AssetList *assets,
    FrameData frame_data, LinearArena *frame_arena);
void game_initialize(GameState *game_state);

#endif //GAME_H
