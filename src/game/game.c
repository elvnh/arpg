#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rgba.h"
#include "renderer/render_batch.h"
#include <stdio.h>

//#include <stdio.h>

static void entity_render(const Entity *entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    RGBA32 color = {0};

    if (entity->collider.is_colliding) {
        color = (RGBA32){1, 0, 0, 1};
    } else {
        color = RGBA32_BLUE;
    }

    render_batch_push_circle(rb, scratch, entity->position, color, entity->collider.radius, assets->shader2, 0);
}

static void world_update(GameWorld *world, const Input *input)
{
    if (world->entity_count == 0) {
        world->entities[world->entity_count++] = (Entity){ {0, 0}, {16.0f, false} };
        world->entities[world->entity_count++] = (Entity){ {64, 0}, {16.0f, false} };
        world->entities[world->entity_count++] = (Entity){ {32, 64}, {8.0f, false} };
    }

    if (input_is_key_down(input, KEY_W)) {
        world->entities[0].position.y += 1.0f;
    } else if (input_is_key_down(input, KEY_S)) {
        world->entities[0].position.y -= 1.0f;
    } else if (input_is_key_down(input, KEY_A)) {
        world->entities[0].position.x -= 1.0f;
    } else if (input_is_key_down(input, KEY_D)) {
        world->entities[0].position.x += 1.0f;
    }

    // Collision
    for (s32 i = 0; i < world->entity_count; ++i) {
        Entity *e = &world->entities[i];
        e->collider.is_colliding = false;
    }

    for (s32 i = 0; i < world->entity_count; ++i) {
        for (s32 j = i + 1; j < world->entity_count; ++j) {
            Entity *a = &world->entities[i];
            Entity *b = &world->entities[j];

            f32 dist = v2_dist(a->position, b->position);
            f32 radii_sum = a->collider.radius + b->collider.radius;

            if (dist <= radii_sum) {
                a->collider.is_colliding = true;
                b->collider.is_colliding = true;
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
