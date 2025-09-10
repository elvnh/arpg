#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "renderer/render_batch.h"
#include <stdio.h>

//#include <stdio.h>

static void entity_render(const Entity *entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    /* if (entity->collider.is_colliding) { */
    /*     color = (RGBA32){1, 0, 0, 1}; */
    /* } else { */
    /*     color = RGBA32_BLUE; */
    /* } */
    Rectangle rect = {
	.position = entity->position,
	.size = entity->size
    };
    render_batch_push_rect(rb, scratch, rect, entity->color, assets->shader2, 0);
}

static void world_update(GameWorld *world, const Input *input)
{
    if (world->entity_count == 0) {
        world->entities[world->entity_count++] = (Entity){ {0, 0}, {0, 0}, {16, 16}, RGBA32_BLUE };
        world->entities[world->entity_count++] = (Entity){ {64, -64}, {-0.0f, 0}, {16, 32}, RGBA32_WHITE };
        //world->entities[world->entity_count++] = (Entity){ {32, 64}, {0, 0}, {32, 16} };
    }

    {
	Vector2 dir = {0};

	f32 velocity = 1.0f;
	if (input_is_key_down(input, KEY_W)) {
	    dir.y = velocity;
	} else if (input_is_key_down(input, KEY_S)) {
	    dir.y = -velocity;
	}

	if (input_is_key_down(input, KEY_A)) {
	    dir.x = -velocity;
	} else if (input_is_key_down(input, KEY_D)) {
	    dir.x = velocity;
	}

	world->entities[0].velocity = dir;
    }

    // Collision


    for (s32 i = 0; i < world->entity_count; ++i) {
        Entity *e = &world->entities[i];
	e->position = v2_add(e->position, e->velocity);
    }
}

static void world_render(const GameWorld *world, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    for (s32 i = 0; i < world->entity_count; ++i) {
        for (s32 j = i + 1; j < world->entity_count; ++j) {
            const Entity *a = &world->entities[i];
            const Entity *b = &world->entities[j];

	    entity_render(a, rb, assets, scratch);
	    entity_render(b, rb, assets, scratch);

	    Rectangle rect_a = { a->position, a->size};
	    Rectangle rect_b = { b->position, b->size};

	    Vector2 tl = v2_sub(rect_top_left(rect_a), rect_bottom_right(rect_b));
	    Vector2 br = v2_sub(rect_bottom_right(rect_a), rect_top_left(rect_b));
	    //Vector2 tr = v2_sub(rect_top_right(rect_a), rect_bottom_left(rect_b));
	    //Vector2 bl = v2_sub(rect_bottom_left(rect_a), rect_top_right(rect_b));

	    if (tl.x <= 0 && br.x >= 0 && tl.y <= 0 && br.y >= 0) {
		printf("C\n");
	    }


/*
black.top_right = red.top_right - blue.bottom_left;
black.top_left = red.top_left - blue.bottom_right;
black.bottom_right = red.bottom_right - blue.top_left;
black.bottom_left = red.bottom_left - blue.top_right;

 */

	    //RGBA32 color = { 0, 0.5f, 0.9f, 0.9f };
	    //render_batch_push_rect(rb, scratch, rect, color, assets->shader2, 0);

        }
    }

    /* for (s32 i = 0; i < world->entity_count; ++i) { */
    /*     entity_render(&world->entities[i], rb, assets, scratch); */
    /* } */
}

static void game_update(GameState *game_state, const Input *input)
{
    world_update(&game_state->world, input);
}

static void game_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets)
{
    world_render(&game_state->world, render_cmds, assets, &game_state->frame_arena);
}

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets,
    const Input *input)
{
    la_reset(&game_state->frame_arena);

    game_update(game_state, input);
    game_render(game_state, render_cmds, assets);
}
