#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/maths.h"
#include "base/utils.h"
#include "base/vector.h"
#include "base/line.h"
#include "game/entity.h"
#include "input.h"
#include "collision.h"
#include "renderer/render_batch.h"

static void entity_render(Entity *entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    // TODO: provide components instead
    Rectangle rect = {
	.position = es_get_component(entity, PhysicsComponent)->position,
	.size = es_get_component(entity, ColliderComponent)->size
    };

    rb_push_rect(rb, scratch, rect, entity->color, assets->shader2, RENDER_LAYER_ENTITIES);
}

static Vector2i world_to_tile_coords(Vector2 world_coords)
{
    Vector2i result = {
        (s32)(world_coords.x / TILE_SIZE),
        (s32)(world_coords.y / TILE_SIZE)
    };

    return result;
}

static Vector2 tile_to_world_coords(Vector2i tile_coords)
{
    Vector2 result = {
        (f32)tile_coords.x * TILE_SIZE,
        (f32)tile_coords.y * TILE_SIZE
    };

    return result;
}

static void entity_vs_entity_collision(PhysicsComponent *physics_a, ColliderComponent *collider_a,
    PhysicsComponent *physics_b, ColliderComponent *collider_b, f32 *movement_fraction_left, f32 dt)
{
    Rectangle rect_a_ = { physics_a->position, collider_a->size};
    Rectangle rect_b_ = { physics_b->position, collider_b->size};

    CollisionInfo collision = collision_rect_vs_rect(*movement_fraction_left, rect_a_, rect_b_,
	physics_a->velocity, physics_b->velocity, dt);

    physics_a->position = collision.new_position_a;
    physics_b->position = collision.new_position_b;

    physics_a->velocity = collision.new_velocity_a;
    physics_b->velocity = collision.new_velocity_b;

    *movement_fraction_left = collision.movement_fraction_left;
}

static void entity_vs_tilemap_collision(PhysicsComponent *physics, ColliderComponent *collider,
    GameWorld *world, f32 *movement_fraction_left, f32 dt)
{
    Vector2 old_entity_pos = physics->position;
    Vector2 new_entity_pos = v2_add(physics->position, v2_mul_s(physics->velocity, dt));
    f32 entity_width = collider->size.x;
    f32 entity_height = collider->size.y;


    f32 min_x = MIN(old_entity_pos.x, new_entity_pos.x);
    f32 max_x = MAX(old_entity_pos.x, new_entity_pos.x) + entity_width;
    f32 min_y = MIN(old_entity_pos.y, new_entity_pos.y);
    f32 max_y = MAX(old_entity_pos.y, new_entity_pos.y) + entity_height;

    s32 min_tile_x = (s32)(min_x / TILE_SIZE);
    s32 max_tile_x = (s32)(max_x / TILE_SIZE);
    s32 min_tile_y = (s32)(min_y / TILE_SIZE);
    s32 max_tile_y = (s32)(max_y / TILE_SIZE);

    for (s32 y = min_tile_y - 1; y <= max_tile_y + 1; ++y) {
        for (s32 x = min_tile_x - 1; x <= max_tile_x + 1; ++x) {
            Vector2i tile_coords = {x, y};
            TileType *tile = tilemap_get_tile(&world->tilemap, tile_coords);

            if (!tile || (*tile == TILE_WALL)) {
                Rectangle entity_rect = { physics->position, collider->size };
                Rectangle tile_rect = {
                    tile_to_world_coords(tile_coords),
                    {(f32)TILE_SIZE, (f32)TILE_SIZE },
                };

                CollisionInfo collision = collision_rect_vs_rect(*movement_fraction_left, entity_rect,
		    tile_rect, physics->velocity, V2_ZERO, dt);

                physics->position = collision.new_position_a;
                physics->velocity = collision.new_velocity_a;
                *movement_fraction_left = collision.movement_fraction_left;

                ASSERT(v2_eq(collision.new_position_b, tile_rect.position));
                ASSERT(v2_eq(collision.new_velocity_b, V2_ZERO));
            }
        }
    }
}

static void handle_collision_and_movement(GameWorld *world, f32 dt)
{
    for (EntityIndex i = 0; i < world->entities.alive_entity_count; ++i) {
        // TODO: this seems bug prone
        EntityID id_a = world->entities.alive_entity_ids[i];
        Entity *a = es_get_entity(&world->entities,id_a);
        PhysicsComponent *physics_a = es_get_component(a, PhysicsComponent);
        ColliderComponent *collider_a = es_get_component(a, ColliderComponent);

        if (!physics_a || !collider_a) {
            continue;
        }

        f32 movement_fraction_left = 1.0f;

        entity_vs_tilemap_collision(physics_a, collider_a, world, &movement_fraction_left, dt);

        for (EntityIndex j = i + 1; j < world->entities.alive_entity_count; ++j) {
            EntityID id_b = world->entities.alive_entity_ids[j];
            Entity *b = es_get_entity(&world->entities, id_b);
            PhysicsComponent *physics_b = es_get_component(b, PhysicsComponent);
            ColliderComponent *collider_b = es_get_component(b, ColliderComponent);

            if (!physics_b || !collider_b) {
                continue;
            }

            entity_vs_entity_collision(physics_a, collider_a, physics_b, collider_b, &movement_fraction_left, dt);
        }

        ASSERT(movement_fraction_left >= 0.0f);
        ASSERT(movement_fraction_left <= 1.0f);

        Vector2 to_move_this_frame = v2_mul_s(v2_mul_s(physics_a->velocity, movement_fraction_left), dt);
        physics_a->position = v2_add(physics_a->position, to_move_this_frame);
    }
}

static void world_update(GameWorld *world, const Input *input, f32 dt)
{
    ASSERT(world->entities.alive_entity_count >= 1);


    EntityID player_id = world->entities.alive_entity_ids[0];
    Entity *player = es_get_entity(&world->entities, player_id);
    PhysicsComponent *physics = es_get_component(player, PhysicsComponent);

    if (physics) {
        world->camera.position = physics->position;
    }


    camera_zoom(&world->camera, (s32)input->scroll_delta);
    camera_update(&world->camera, dt);

    f32 speed = 250.0f;

    if (input_is_key_held(input, KEY_LEFT_SHIFT)) {
        speed *= 3.0f;
    }

    {
	Vector2 dir = {0};

	if (input_is_key_down(input, KEY_W)) {
	    dir.y = 1.0f;
	} else if (input_is_key_down(input, KEY_S)) {
	    dir.y = -1.0f;
	}

	if (input_is_key_down(input, KEY_A)) {
	    dir.x = -1.0f;
	} else if (input_is_key_down(input, KEY_D)) {
	    dir.x = 1.0f;
	}

        physics->velocity = v2_mul_s(v2_norm(dir), speed);
    }

    handle_collision_and_movement(world, dt);
}

static void world_render(GameWorld *world, RenderBatch *rb, const AssetList *assets, FrameData frame_data,
    LinearArena *frame_arena)
{


    for (s32 y = 0; y < TILEMAP_HEIGHT; ++y) {
        for (s32 x = 0; x < TILEMAP_WIDTH; ++x) {
            Vector2i tile_coords = {x, y};
            TileType *tile = tilemap_get_tile(&world->tilemap, tile_coords);
            RGBA32 color;

            if (tile) {
                switch (*tile) {
                    case TILE_FLOOR: {
                        color = (RGBA32){0.1f, 0.5f, 0.9f, 1.0f};
                    } break;

                    case TILE_WALL: {
                        color = (RGBA32){1.0f, 0.2f, 0.1f, 1.0f};
                    } break;

                    INVALID_DEFAULT_CASE;
                }
            } else {
                color = RGBA32_WHITE;
            }

            Rectangle tile_rect = {
                .position = {(f32)(x * TILE_SIZE), (f32)(y * TILE_SIZE)},
                .size = { (f32)TILE_SIZE, (f32)TILE_SIZE }
            };

            rb_push_rect(rb, frame_arena, tile_rect, color, assets->shader2, RENDER_LAYER_TILEMAP);
        }
    }

    for (EntityIndex i = 0; i < world->entities.alive_entity_count; ++i) {
        EntityID id = world->entities.alive_entity_ids[i];
        Entity *entity = es_get_entity(&world->entities, id);

        entity_render(entity, rb, assets, frame_arena);
    }
}

static void game_update(GameState *game_state, const Input *input, f32 dt, LinearArena *frame_arena)
{
    (void)frame_arena;

    if (input_is_key_pressed(input, KEY_ESCAPE)) {
        DEBUG_BREAK;
    }

    world_update(&game_state->world, input, dt);
}

static void game_render(GameState *game_state, RenderBatchList *rbs, const AssetList *assets,
    FrameData frame_data, LinearArena *frame_arena)
{
    Matrix4 proj = camera_get_matrix(game_state->world.camera, frame_data.window_width, frame_data.window_height);
    RenderBatch *rb = rb_list_push_new(rbs, proj, frame_arena);

    world_render(&game_state->world, rb, assets, frame_data, frame_arena);

    rb_sort_entries(rb);
}

void game_update_and_render(GameState *game_state, RenderBatchList *rbs, const AssetList *assets,
    FrameData frame_data, LinearArena *frame_arena)
{
    game_update(game_state, frame_data.input, frame_data.dt, frame_arena);
    game_render(game_state, rbs, assets, frame_data, frame_arena);
}

void game_initialize(GameState *game_state)
{
    es_initialize(&game_state->world.entities);

    for (s32 i = 0; i < 2; ++i) {
        EntityID id = es_create_entity(&game_state->world.entities);
        Entity *player = es_get_entity(&game_state->world.entities, id);

        ASSERT(!es_has_component(player, PhysicsComponent));
        ASSERT(!es_has_component(player, ColliderComponent));

        player->color = RGBA32_BLUE;
        PhysicsComponent *physics = es_add_component(player, PhysicsComponent);
        physics->position = v2((f32)(16 * i), 16.0f);

        ColliderComponent *collider = es_add_component(player, ColliderComponent);
        collider->size = v2(32.0f, (f32)(16 * (i + 1)));

        ASSERT(es_has_component(player, PhysicsComponent));
        ASSERT(es_has_component(player, ColliderComponent));
    }

    /* EntityID other_id = es_create_entity(&game_state->world.entities); */
    /* Entity *other = es_get_entity(&game_state->world.entities, other_id); */

    /* other->position = v2(32, 32); */
    /* other->size = (Vector2){16, 32}; */
    /* other->color = RGBA32_RED;       */

    game_state->world.tilemap.tiles[0] = TILE_WALL;
    game_state->world.tilemap.tiles[1] = TILE_WALL;
    game_state->world.tilemap.tiles[2] = TILE_WALL;
    game_state->world.tilemap.tiles[3] = TILE_WALL;
}
