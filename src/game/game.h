#ifndef GAME_H
#define GAME_H

#include "renderer/render_batch.h"

typedef struct {
    LinearArena frame_arena;
} GameState;

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets);
//void game_initialize(GameState *game_state);

#endif //GAME_H
