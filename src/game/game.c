#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"

static void game_update()
{

}

static void game_render(RenderBatch *render_cmds)
{
    (void)render_cmds;
}

void game_update_and_render(RenderBatch *render_cmds)
{
    game_update();
    game_render(render_cmds);
}
