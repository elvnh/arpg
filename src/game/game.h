#ifndef GAME_H
#define GAME_H

#include "renderer/render_batch.h"

typedef struct {
    LinearArena frame_arena;
    ShaderHandle shader;
    TextureHandle texture;
} GameState;

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds);

#endif //GAME_H
