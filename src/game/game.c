#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "renderer/render_batch.h"

#include <stdio.h>


static void entity_render(const Entity *entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    Rectangle rect = {
	.position = entity->position,
	.size = entity->size
    };
    render_batch_push_rect(rb, scratch, rect, entity->color, assets->shader2, 0);
}

static Vector2 closest_point_on_rect_bounds(Rectangle rect, Vector2 point)
{
    f32 min_dist = (f32)fabs(point.x - rect.position.x);
    Vector2 bounds_point = { rect.position.x, point.y };

    f32 max_x = rect.position.x + rect.size.x;
    f32 max_y = rect.position.y + rect.size.y;
    f32 min_y = rect.position.y;

    if ((f32)fabs(max_x - point.x) < min_dist) {
        min_dist = (f32)fabs(max_x - point.x);
        bounds_point = (Vector2){ max_x, point.y };
    }

    if ((f32)fabs(max_y - point.y) < min_dist) {
        min_dist = (f32)fabs(max_y - point.y);
        bounds_point = (Vector2){ point.x, max_y };
    }

    if ((f32)fabs(min_y - point.y) < min_dist) {
        min_dist = (f32)fabs(min_y - point.y);
        bounds_point = (Vector2){point.x, min_y};
    }

    return bounds_point;
}

static void world_update(GameWorld *world, const Input *input)
{
    if (world->entity_count == 0) {
        world->entities[world->entity_count++] = (Entity){ {0, 0}, {0, 0}, {16, 16}, RGBA32_BLUE };
        world->entities[world->entity_count++] = (Entity){ {64, 0}, {-0.0f, 0}, {16, 32}, RGBA32_WHITE };
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

    for (s32 i = 0; i < world->entity_count; ++i) {
        for (s32 j = i + 1; j < world->entity_count; ++j) {
            Entity *a = &world->entities[i];
            Entity *b = &world->entities[j];

	    Rectangle rect_a = { a->position, a->size};
	    Rectangle rect_b = { b->position, b->size};

            f32 width = rect_a.size.x + rect_b.size.x;
            f32 height = rect_a.size.y + rect_b.size.y;
            Vector2 size = { width, height };

            Vector2 left = v2_sub(rect_a.position, rect_bottom_right(rect_b));
            Vector2 bottom = v2_sub(rect_top_left(rect_b), rect_bottom_left(rect_a));
            Vector2 right = v2_add(left, size);
            Vector2 top = v2_add(bottom, size);

            Vector2 pos = { left.x, left.y };

            Rectangle rect = { pos, size };

            if (left.x <= 0 && right.x >= 0 && bottom.y <= 0 && top.y >= 0) {
                Vector2 pen = closest_point_on_rect_bounds(rect, (Vector2){ 0 });
                a->position = v2_sub(a->position, pen);
            }
        }
    }
}

static void world_render(const GameWorld *world, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    for (s32 i = 0; i < world->entity_count; ++i) {
        entity_render(&world->entities[i], rb, assets, scratch);
    }

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
