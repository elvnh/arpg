#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "game/renderer/render_batch.h"

#include <stdio.h>

static void game_update(GameState *game_state)
{
    (void)game_state;
    printf("hej\n");
}

static void game_render(GameState *game_state, RenderBatch *render_cmds)
{
    (void)render_cmds;
    (void)game_state;

#if 0
    Rectangle rect = {
        .position = {0},
        .size = {64, 64}
    };

    rb_push_sprite(render_cmds, &game_state->frame_arena, game_state->texture,
        rect, game_state->shader, 0);
#endif
}

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds)
{
    la_reset(&game_state->frame_arena);

    game_update(game_state);
    game_render(game_state, render_cmds);
}
