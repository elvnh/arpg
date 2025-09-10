#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rgba.h"
#include "renderer/render_batch.h"

//#include <stdio.h>

static void game_update(GameState *game_state)
{
    (void)game_state;
    //printf("hej\n");
}

static void game_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets)
{
    Rectangle rect = {
        .position = {0, 10},
        .size = {64, 64}
    };

    /* render_batch_push_quad(render_cmds, &game_state->frame_arena, (Rectangle){{0, 16}, {32, 32}}, */
    /*     (RGBA32){0, 1, 1, 0.5f}, assets->shader, 0); */

    render_batch_push_circle(render_cmds, &game_state->frame_arena, rect.position, RGBA32_WHITE, 64.0f, assets->shader2, 0);

    render_batch_push_sprite_circle(render_cmds, &game_state->frame_arena, assets->texture, (Vector2){0, 0}, RGBA32_WHITE,
        32.0f, assets->shader, 0);
}

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets)
{
    la_reset(&game_state->frame_arena);

    game_update(game_state);
    game_render(game_state, render_cmds, assets);
}
