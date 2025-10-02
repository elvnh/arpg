#ifndef GAME_H
#define GAME_H

#include "base/matrix.h"
#include "game/ui.h"
#include "renderer/render_batch.h"
#include "game_world.h"
#include "input.h"

/*
  TODO:
  - Manually controlling dt to slow down/speed up time
 */

typedef struct {
    GameWorld world;
    UIState ui;

    // TODO: these shouldn't be here
    TextureHandle texture;
    FontHandle font;
} GameState;

typedef struct {
    f32 dt;
    const Input *input;
    s32 window_width;
    s32 window_height;
} FrameData;

typedef struct {
    LinearArena permanent_memory;
    LinearArena temporary_memory;
} GameMemory;

void game_update_and_render(GameState *game_state, RenderBatchList *rbs, const AssetList *assets,
    FrameData frame_data, GameMemory *game_memory);
void game_initialize(GameState *game_state, GameMemory *game_memory);

#endif //GAME_H
