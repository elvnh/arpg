#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/maths.h"
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

typedef struct {
    b32     are_colliding;
    Vector2 collision_vector;
} RectangleCollision;

static RectangleCollision rectangles_collide_discrete(Rectangle a, Rectangle b)
{
    RectangleCollision result = {0};

    Vector2 size = v2_add(a.size, b.size);
    Vector2 left = v2_sub(a.position, rect_bottom_right(b));
    Vector2 bottom = v2_sub(rect_top_left(b), rect_bottom_left(a));
    Vector2 right = v2_add(left, size);
    Vector2 top = v2_add(bottom, size);

    Vector2 pos = { left.x, left.y };
    Rectangle rect = { pos, size };

    if (left.x <= 0 && right.x >= 0 && bottom.y <= 0 && top.y >= 0) {
        result.are_colliding = true;
        result.collision_vector = rect_bounds_point_closest_to_point(rect, (Vector2){ 0 });
    }

    return result;
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

            RectangleCollision collision = rectangles_collide_discrete(rect_a, rect_b);

            if (collision.are_colliding) {
                a->position = v2_add(a->position, collision.collision_vector);
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
