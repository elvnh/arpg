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
#include "input.h"
#include "renderer/render_batch.h"

#define INTERSECTION_EPSILON   0.00001f
#define COLLISION_MARGIN       0.001f

static void entity_render(const Entity *entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    Rectangle rect = {
	.position = entity->position,
	.size = entity->size
    };

    render_batch_push_rect(rb, scratch, rect, entity->color, assets->shader2, 0);
}

typedef struct {
    Vector2 new_position_a;
    Vector2 new_position_b;
    Vector2 new_velocity_a;
    Vector2 new_velocity_b;
    f32 movement_fraction_left;
} CollisionInfo;

static CollisionInfo collision_rect_vs_rect(f32 movement_fraction_left, Rectangle rect_a, Rectangle rect_b,
    Vector2 velocity_a, Vector2 velocity_b, f32 dt)
{
    ASSERT(movement_fraction_left >= 0.0f);

    CollisionInfo result = {
        .new_position_a = rect_a.position,
        .new_position_b = rect_b.position,
        .new_velocity_a = velocity_a,
        .new_velocity_b = velocity_b,
        .movement_fraction_left = movement_fraction_left
    };

    Rectangle md = rect_minkowski_diff(rect_a, rect_b);
    Vector2 collision_vector = rect_bounds_point_closest_to_point(md, V2_ZERO);

    if (rect_contains_point(md, V2_ZERO) && (v2_mag(collision_vector) > INTERSECTION_EPSILON)) {
        // Have already collided
        result.new_position_a = v2_sub(result.new_position_a, collision_vector);

        // TODO: only reset velocity in collision direction
        result.new_velocity_a = V2_ZERO;
        result.new_velocity_b = V2_ZERO;
    } else {
        Vector2 a_movement = v2_mul_s(velocity_a, movement_fraction_left);
        Vector2 relative_movement = v2_mul_s(v2_sub(velocity_b, a_movement), dt);
        Line ray_line = { V2_ZERO, relative_movement };

        RectRayIntersection intersection = rect_shortest_ray_intersection(md, ray_line, INTERSECTION_EPSILON);

        if (intersection.is_intersecting) {
            Vector2 a_dist_to_move = v2_mul_s(velocity_a, intersection.time_of_impact);
            Vector2 b_dist_to_move =  v2_mul_s(velocity_b, intersection.time_of_impact);

            result.new_position_a = v2_add(rect_a.position, v2_mul_s(a_dist_to_move, intersection.time_of_impact * dt));
            result.new_position_b = v2_add(rect_b.position, v2_mul_s(b_dist_to_move, intersection.time_of_impact * dt));

            // Move slightly in opposite direction to prevent getting stuck
            result.new_position_a = v2_add(result.new_position_a, v2_mul_s(v2_norm(velocity_a), -COLLISION_MARGIN));
            result.new_position_b = v2_add(result.new_position_b, v2_mul_s(v2_norm(velocity_b), -COLLISION_MARGIN));

            if ((intersection.side_of_collision == RECT_SIDE_LEFT)
             || (intersection.side_of_collision == RECT_SIDE_RIGHT)) {
                // TODO: this makes diagonal movement into walls slower, instead transfer velocity from other axis
                result.new_velocity_a.x = 0.0f;
                result.new_velocity_b.x = 0.0f;
            } else {
                result.new_velocity_a.y = 0.0f;
                result.new_velocity_b.y = 0.0f;
            }

            result.movement_fraction_left = MAX(0.0f, result.movement_fraction_left - intersection.time_of_impact);
        }
    }

    return result;
}

static void entity_vs_entity_collision(Entity *a, Entity *b, f32 *movement_fraction_left, f32 dt)
{
    Rectangle rect_a_ = { a->position, a->size};
    Rectangle rect_b_ = { b->position, b->size};

    CollisionInfo collision = collision_rect_vs_rect(*movement_fraction_left, rect_a_, rect_b_, a->velocity, b->velocity, dt);

    a->position = collision.new_position_a;
    b->position = collision.new_position_b;
    a->velocity = collision.new_velocity_a;
    b->velocity = collision.new_velocity_b;
    *movement_fraction_left = collision.movement_fraction_left;
}

static void entity_vs_tilemap_collision(Entity *entity, GameWorld *world, f32 *movement_fraction_left, f32 dt)
{
    Rectangle entity_rect = { entity->position, entity->size };

    s32 min_tile_x = (s32)(entity->position.x / TILE_SIZE) - 1;
    s32 max_tile_x = (s32)(entity->position.x / TILE_SIZE) + (s32)(entity->size.x / TILE_SIZE) + 1;
    s32 min_tile_y = (s32)(entity->position.y / TILE_SIZE) - 1;
    s32 max_tile_y = (s32)(entity->position.y / TILE_SIZE) + (s32)(entity->size.y / TILE_SIZE) + 1;

    for (s32 y = min_tile_y; y <= max_tile_y; ++y) {
        for (s32 x = min_tile_x; x <= max_tile_x; ++x) {
            TileType *tile = tilemap_get_tile(&world->tilemap, x, y);

            if (tile) {
                Rectangle tile_rect = {
                    {(f32)(x * TILE_SIZE), (f32)(y * TILE_SIZE) },
                    {(f32)TILE_SIZE, (f32)TILE_SIZE },
                };

                CollisionInfo collision = collision_rect_vs_rect(*movement_fraction_left, entity_rect, tile_rect,
                    entity->velocity, V2_ZERO, dt);

                entity->position = collision.new_position_a;
                entity->velocity = collision.new_velocity_a;
                *movement_fraction_left = collision.movement_fraction_left;

                ASSERT(v2_eq(collision.new_position_b, tile_rect.position));
                ASSERT(v2_eq(collision.new_velocity_b, V2_ZERO));
            }
        }
    }
}

static void handle_collision_and_movement(GameWorld *world, f32 dt)
{
    for (s32 i = 0; i < world->entity_count; ++i) {
        Entity *a = &world->entities[i];

        f32 movement_fraction_left = 1.0f;

        entity_vs_tilemap_collision(a, world, &movement_fraction_left, dt);

        for (s32 j = i + 1; j < world->entity_count; ++j) {
            Entity *b = &world->entities[j];

            entity_vs_entity_collision(a, b, &movement_fraction_left, dt);
        }

        ASSERT(movement_fraction_left >= 0.0f);
        ASSERT(movement_fraction_left <= 1.0f);

        Vector2 to_move_this_frame = v2_mul_s(v2_mul_s(a->velocity, movement_fraction_left), dt);
        a->position = v2_add(a->position, to_move_this_frame);
    }
}

static void world_update(GameWorld *world, const Input *input, f32 dt)
{
    if (world->entity_count == 0) {
        world->entities[world->entity_count++] = (Entity){ {32, 32}, {0, 0}, {16, 16}, RGBA32_BLUE };
        world->entities[world->entity_count++] = (Entity){ {64, 64}, {-0.0f, 0}, {16, 32}, RGBA32_RED };
        /* world->entities[world->entity_count++] = (Entity){ {32, 32}, {0, 0}, {32, 16}, RGBA32_RED}; */

        world->tilemap.tiles[0] = TILE_FLOOR;
        world->tilemap.tiles[1] = TILE_WALL;
    }

    world->camera.position = world->entities[0].position;

    f32 speed = 100.0f;
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

	world->entities[0].velocity = v2_mul_s(v2_norm(dir), speed);
    }

    {
	Vector2 dir = {0};

	if (input_is_key_down(input, KEY_UP)) {
            camera_change_zoom(&world->camera, 0.1f);
	    dir.y = 1.0f;
	} else if (input_is_key_down(input, KEY_DOWN)) {
            camera_change_zoom(&world->camera, -0.1f);
	    dir.y = -1.0f;
	}

	if (input_is_key_down(input, KEY_LEFT)) {
	    dir.x = -1.0f;
	} else if (input_is_key_down(input, KEY_RIGHT)) {
	    dir.x = 1.0f;
	}

	world->entities[1].velocity = v2_mul_s(v2_norm(dir), speed);
    }

    /* if (input_is_key_pressed(input, KEY_D) || input_is_key_pressed(input, KEY_A) */
    /*     || input_is_key_pressed(input, KEY_S) ||input_is_key_pressed(input, KEY_W) */
    /*     || input_is_key_pressed(input, KEY_LEFT) || input_is_key_pressed(input, KEY_RIGHT) */
    /*     || input_is_key_pressed(input, KEY_UP) ||input_is_key_pressed(input, KEY_DOWN)       ) { */
    /*     printf("d\n"); */
    /* } */

    handle_collision_and_movement(world, dt);
}

static void world_render(GameWorld *world, RenderBatch *rb, const AssetList *assets, FrameData frame_data,
    LinearArena *frame_arena)
{
    rb->projection = camera_get_matrix(world->camera, frame_data.window_width, frame_data.window_height);

    for (s32 y = 0; y < TILEMAP_HEIGHT; ++y) {
        for (s32 x = 0; x < TILEMAP_WIDTH; ++x) {
            TileType *tile = tilemap_get_tile(&world->tilemap, x, y);
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

            render_batch_push_rect(rb, frame_arena, tile_rect, color, assets->shader2, 0);
        }
    }

    for (s32 i = 0; i < world->entity_count; ++i) {
        entity_render(&world->entities[i], rb, assets, frame_arena);
    }
}

static void game_update(GameState *game_state, const Input *input, f32 dt, LinearArena *frame_arena)
{
    (void)frame_arena;
    world_update(&game_state->world, input, dt);
}

static void game_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets,
    FrameData frame_data, LinearArena *frame_arena)
{
    world_render(&game_state->world, render_cmds, assets, frame_data, frame_arena);
}

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets,
    FrameData frame_data, LinearArena *frame_arena)
{
    game_update(game_state, frame_data.input, frame_data.dt, frame_arena);
    game_render(game_state, render_cmds, assets, frame_data, frame_arena);
}
