#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/maths.h"
#include "base/vector.h"
#include "base/line.h"
#include "input.h"
#include "renderer/render_batch.h"

#define INTERSECTION_EPSILON   0.00001f
#define COLLISION_MARGIN       0.01f

static void entity_render(const Entity *entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    Rectangle rect = {
	.position = entity->position,
	.size = entity->size
    };

    render_batch_push_rect(rb, scratch, rect, entity->color, assets->shader2, 0);
}

static void handle_collision_and_movement(GameWorld *world, f32 dt)
{

    for (s32 i = 0; i < world->entity_count; ++i) {
        Entity *a = &world->entities[i];

        f32 movement_fraction_left = 1.0f;

        for (s32 j = i + 1; j < world->entity_count; ++j) {
            Entity *b = &world->entities[j];

	    Rectangle rect_a = { a->position, a->size};
	    Rectangle rect_b = { b->position, b->size};

            Rectangle md = rect_minkowski_diff(rect_a, rect_b);
            Vector2 collision_vector = rect_bounds_point_closest_to_point(md, V2_ZERO);

            if (rect_contains_point(md, V2_ZERO) && (v2_mag(collision_vector) > INTERSECTION_EPSILON)) {
                // Have already collided
                // TODO: should only velocity in collision direction be reset here?
                a->position = v2_sub(a->position, collision_vector);

                a->velocity = V2_ZERO;
                b->velocity = V2_ZERO;
            } else {
                Vector2 relative_velocity = v2_mul_s(v2_sub(b->velocity, a->velocity), dt);
                Line ray_line = { V2_ZERO, relative_velocity };
                RectRayIntersection intersection = rect_shortest_ray_intersection(md, ray_line,
                    INTERSECTION_EPSILON);

                if (intersection.is_intersecting) {
                    Vector2 a_dist_to_collision = v2_mul_s(a->velocity, intersection.time_of_impact);
                    Vector2 b_dist_to_collision =  v2_mul_s(b->velocity, intersection.time_of_impact);

                    Vector2 new_pos_a = v2_add(a->position, v2_mul_s(a_dist_to_collision, dt));
                    Vector2 new_pos_b = v2_add(b->position, v2_mul_s(b_dist_to_collision, dt));

                    // Move slightly in opposite direction to prevent getting stuck
                    a->position = v2_add(new_pos_a, v2_mul_s(v2_norm(a->velocity), -COLLISION_MARGIN));
                    b->position = v2_add(new_pos_b, v2_mul_s(v2_norm(b->velocity), -COLLISION_MARGIN));

                    if ((intersection.side_of_collision == RECT_SIDE_LEFT)
                        || (intersection.side_of_collision == RECT_SIDE_RIGHT)) {
                        // TODO: this makes diagonal movement into walls slower, instead transfer velocity from other axis
                        a->velocity.x = 0.0f;
                        b->velocity.x = 0.0f;
                    } else {
                        a->velocity.y = 0.0f;
                        b->velocity.y = 0.0f;
                    }

                    movement_fraction_left = 1.0f - intersection.time_of_impact;
                }
            }
        }

        Vector2 to_move_this_frame = v2_mul_s(v2_mul_s(a->velocity, movement_fraction_left), dt);
        a->position = v2_add(a->position, to_move_this_frame);
    }
}

static void world_update(GameWorld *world, const Input *input, f32 dt)
{
    if (world->entity_count == 0) {
        world->entities[world->entity_count++] = (Entity){ {0, 0}, {0, 0}, {16, 16}, RGBA32_BLUE };
        world->entities[world->entity_count++] = (Entity){ {64, 0}, {-0.0f, 0}, {16, 32}, RGBA32_WHITE };
        world->entities[world->entity_count++] = (Entity){ {32, 32}, {0, 0}, {32, 16}, RGBA32_RED};
    }

    world->camera.position = world->entities[0].position;

    f32 speed = 10.0f;
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
            world->camera.zoom += 0.1f;
	    dir.y = 1.0f;
	} else if (input_is_key_down(input, KEY_DOWN)) {
            world->camera.zoom -= 0.1f;
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

static void world_render(const GameWorld *world, RenderBatch *rb, const AssetList *assets, FrameData frame_data,
    LinearArena *scratch)
{
    rb->projection = camera_get_matrix(world->camera, frame_data.window_width, frame_data.window_height);

    for (s32 i = 0; i < world->entity_count; ++i) {
        entity_render(&world->entities[i], rb, assets, scratch);
    }
}

static void game_update(GameState *game_state, const Input *input, f32 dt)
{
    world_update(&game_state->world, input, dt);
}

static void game_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets,
    FrameData frame_data)
{
    world_render(&game_state->world, render_cmds, assets, frame_data, &game_state->frame_arena);
}

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets,
    FrameData frame_data)
{
    la_reset(&game_state->frame_arena);

    game_update(game_state, frame_data.input, frame_data.dt);
    game_render(game_state, render_cmds, assets, frame_data);
}
