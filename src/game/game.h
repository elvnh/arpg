#ifndef GAME_H
#define GAME_H

#include "base/matrix.h"
#include "base/random.h"
#include "game/magic.h"
#include "ui/ui_core.h"
#include "platform.h"
#include "renderer/render_batch.h"
#include "world.h"
#include "input.h"
#include "debug.h"
#include "animation.h"

/*
  TODO:
  - Should asset list be a global variable?
 */

typedef struct {
    World world;
    AssetTable asset_list;
    DebugState debug_state;
    SpellArray spells;
    RNGState rng_state;
    AnimationTable animations;
    UIState ui;
} GameState;

typedef struct {
    LinearArena permanent_memory;
    LinearArena temporary_memory;
} GameMemory;

void game_update_and_render(GameState *game_state, PlatformCode platform_code, RenderBatchList *rbs,
    FrameData frame_data, GameMemory *game_memory);
void game_initialize(GameState *game_state, GameMemory *game_memory);

#endif //GAME_H
