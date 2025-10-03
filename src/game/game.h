#ifndef GAME_H
#define GAME_H

#include "base/matrix.h"
#include "ui/ui_core.h"
#include "platform.h"
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
    AssetList asset_list;
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

void game_update_and_render(GameState *game_state, PlatformCode platform_code, RenderBatchList *rbs,
    FrameData frame_data, GameMemory *game_memory);
void game_initialize(GameState *game_state, GameMemory *game_memory);

#endif //GAME_H
