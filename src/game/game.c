#include "game.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/maths.h"
#include "base/vector.h"
#include "input.h"
#include "renderer/render_batch.h"
#include <math.h>

#define INTERSECTION_EPSILON 0.00001f
#define COLLISION_MARGIN     0.01f

typedef struct {
    Vector2 start;
    Vector2 end;
} Line;

typedef struct {
    bool are_intersecting;
    Vector2 intersection_point;
} LineIntersection;

// Taken from: https://blog.hamaluik.ca/posts/swept-aabb-collision-using-minkowski-difference/
static LineIntersection line_intersection(Line a, Line b, f32 epsilon)
{
    LineIntersection result = {0};

    Vector2 r = v2_sub(a.end, a.start);
    Vector2 s = v2_sub(b.end, b.start);

    f32 numerator = v2_cross(v2_sub(b.start, a.start), r);
    f32 denominator = v2_cross(r, s);

    if ((numerator == 0.0f) && (denominator == 0.0f)) {
        // Lines are colinear so they might still overlap
        // TODO: check if they overlap
    } else if (denominator != 0.0f) {
        f32 u = numerator / denominator;
        f32 t = v2_cross(v2_sub(b.start, a.start), s) / denominator;

        if (f32_in_range(u, 0.0f, 1.0f, epsilon) && f32_in_range(t, 0.0f, 1.0f, epsilon)) {
            Vector2 intersection = {
                a.start.x + r.x * t,
                b.start.y + s.y * u,
            };

            result.are_intersecting = true;
            result.intersection_point = intersection;
        }
    }

    return result;
}

static void entity_render(const Entity *entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    Rectangle rect = {
	.position = entity->position,
	.size = entity->size
    };

    render_batch_push_rect(rb, scratch, rect, entity->color, assets->shader2, 0);
}

static Line line_a = { {0.0f, 0.0f}, {64.0f, 0.0f} };
static Line line_b = { {20.0f, -10.0f}, {20.0f, 20.0f} };

typedef struct {
    b32     are_colliding;
    Vector2 collision_vector;
} RectangleCollision;

// Taken from: https://blog.hamaluik.ca/posts/simple-aabb-collision-using-minkowski-difference/
RectangleCollision rectangles_collide_discrete(Rectangle a, Rectangle b)
{
    RectangleCollision result = {0};

    Vector2 size = v2_add(a.size, b.size);

    Vector2 left = v2_sub(a.position, rect_bottom_right(b));
    Vector2 bottom = v2_sub(rect_top_left(a), rect_bottom_left(b));
    Vector2 right = v2_add(left, size);
    Vector2 top = v2_add(bottom, size);

    Vector2 pos = { left.x, left.y };
    Rectangle rect = { pos, size };

    if (left.x <= 0 && right.x >= 0 && bottom.y <= 0 && top.y >= 0) {
        result.are_colliding = true;
        result.collision_vector = rect_bounds_point_closest_to_point(rect, V2_ZERO);
    }

    return result;
}

void collision_response_rectangles_discrete(RectangleCollision collision, Entity *a, Entity *b)
{
    b32 a_moving = v2_mag(a->velocity) > 0.0f;
    b32 b_moving = v2_mag(b->velocity) > 0.0f;

    if ((a_moving && b_moving) || (!a_moving && !b_moving)) {
        // If either both are moving or neither are moving, move half of the distance each
        Vector2 a_vec = v2_div_s(collision.collision_vector, 2);
        Vector2 b_vec = v2_div_s(collision.collision_vector, 2);

        a->position = v2_sub(a->position, a_vec);
        b->position = v2_add(b->position, b_vec);
    } else if (a_moving) {
        a->position = v2_sub(a->position, collision.collision_vector);
    } else if (b_moving) {
        b->position = v2_add(b->position, collision.collision_vector);
    }
}

static f32 ray_vs_line_intersection_fraction(Vector2 ray_origin, Vector2 ray_end, Line line)
{
    // TODO: don't use sentinel value
    Line ray_line = { ray_origin, ray_end };
    LineIntersection intersection = line_intersection(ray_line, line, INTERSECTION_EPSILON);

    if (intersection.are_intersecting) {
        // TODO: use squared distances
        f32 ray_length = v2_dist(ray_origin, ray_end);
        f32 fraction = v2_dist(intersection.intersection_point, ray_origin) / ray_length;

        return fraction;
    }

    return INFINITY;
}

static f32 rect_min_ray_intersection_fraction(Rectangle rect, Vector2 origin, Vector2 direction)
{
    Vector2 end = v2_add(origin, direction);

    Line left = {
        rect.position,
        v2(rect.position.x, rect.position.y + rect.size.y)
    };

    Line top = {
        v2(rect.position.x, rect.position.y + rect.size.y),
        v2(rect.position.x + rect.size.x, rect.position.y + rect.size.y)
    };

    Line right = {
        v2(rect.position.x + rect.size.x, rect.position.y),
        v2(rect.position.x + rect.size.x, rect.position.y + rect.size.y)
    };

    Line bottom = {
        v2(rect.position.x, rect.position.y),
        v2(rect.position.x + rect.size.x, rect.position.y)
    };

    f32 min_fraction = ray_vs_line_intersection_fraction(origin, end, left);

    min_fraction = MIN(min_fraction, ray_vs_line_intersection_fraction(origin, end, top));
    min_fraction = MIN(min_fraction, ray_vs_line_intersection_fraction(origin, end, right));
    min_fraction = MIN(min_fraction, ray_vs_line_intersection_fraction(origin, end, bottom));

    return min_fraction;
}

static Rectangle minkowski_diff(Rectangle a, Rectangle b)
{
    Vector2 size = v2_add(a.size, b.size);
    Vector2 left = v2_sub(a.position, rect_bottom_right(b));
    Vector2 pos = { left.x, left.y };

    Rectangle result = { pos, size };

    return result;
}

static void world_update(GameWorld *world, const Input *input, f32 dt)
{
    if (world->entity_count == 0) {
        world->entities[world->entity_count++] = (Entity){ {0, 0}, {0, 0}, {16, 16}, RGBA32_BLUE };
        world->entities[world->entity_count++] = (Entity){ {64, 0}, {-0.0f, 0}, {16, 32}, RGBA32_WHITE };
        //world->entities[world->entity_count++] = (Entity){ {32, 64}, {0, 0}, {32, 16} };
    }

    f32 speed = 10.0f;
    {
	Vector2 dir = {0};

	if (input_is_key_down(input, KEY_W)) {
	    dir.y = speed;
	} else if (input_is_key_down(input, KEY_S)) {
	    dir.y = -speed;
	}

	if (input_is_key_down(input, KEY_A)) {
	    dir.x = -speed;
	} else if (input_is_key_down(input, KEY_D)) {
	    dir.x = speed;
	}

	world->entities[0].velocity = dir;
        line_a.start = v2_add(line_a.start, v2_mul_s(dir, dt));
    }

    {
	Vector2 dir = {0};

	if (input_is_key_down(input, KEY_UP)) {
	    dir.y = speed;
            //world->entities[0].position = V2_ZERO;
	} else if (input_is_key_down(input, KEY_DOWN)) {
	    dir.y = -speed;
	}

	if (input_is_key_down(input, KEY_LEFT)) {
	    dir.x = -speed;
	} else if (input_is_key_down(input, KEY_RIGHT)) {
	    dir.x = speed;
	}

	world->entities[1].velocity = dir;
        line_a.end = v2_add(line_a.end, v2_mul_s(dir, dt));
    }

    // Collision

    if (input_is_key_pressed(input, KEY_D) || input_is_key_pressed(input, KEY_A)
        || input_is_key_pressed(input, KEY_S) ||input_is_key_pressed(input, KEY_W)
        || input_is_key_pressed(input, KEY_LEFT) || input_is_key_pressed(input, KEY_RIGHT)
        || input_is_key_pressed(input, KEY_UP) ||input_is_key_pressed(input, KEY_DOWN)       ) {
        printf("d\n");
    }

    for (s32 i = 0; i < world->entity_count; ++i) {
        Entity *a = &world->entities[i];
        Vector2 velocity_this_frame = a->velocity;

        for (s32 j = i + 1; j < world->entity_count; ++j) {
            Entity *b = &world->entities[j];

	    Rectangle rect_a = { a->position, a->size};
	    Rectangle rect_b = { b->position, b->size};

            Rectangle md = minkowski_diff(rect_a, rect_b);
            Vector2 collision_vector = rect_bounds_point_closest_to_point(md, V2_ZERO);

            if (rect_contains_point(md, V2_ZERO) && (v2_mag(collision_vector) > INTERSECTION_EPSILON)) {
                // Have already collided
                printf("c\n");
                a->position = v2_sub(a->position, collision_vector);

                velocity_this_frame = V2_ZERO;

                a->velocity = V2_ZERO;
                b->velocity = V2_ZERO;
            } else {
                Vector2 relative_velocity = v2_mul_s(v2_sub(b->velocity, a->velocity), dt);
                f32 intersect_fraction = rect_min_ray_intersection_fraction(md, V2_ZERO, relative_velocity);

                if (intersect_fraction != INFINITY) {
                    Vector2 new_pos_a = v2_add(a->position, v2_mul_s(v2_mul_s(a->velocity, intersect_fraction), dt));
                    Vector2 new_pos_b = v2_add(b->position, v2_mul_s(v2_mul_s(b->velocity, intersect_fraction), dt));

                    // Move slightly in opposite direction to prevent getting stuck
                    a->position = v2_add(new_pos_a, v2_mul_s(v2_norm(a->velocity), -COLLISION_MARGIN));
                    b->position = v2_add(new_pos_b, v2_mul_s(v2_norm(b->velocity), -COLLISION_MARGIN));

                    Vector2 normalized_rel_velocity = v2_norm(relative_velocity);
                    Vector2 tangent = { -normalized_rel_velocity.y, normalized_rel_velocity.x };

                    a->velocity = v2_mul_s(tangent, v2_dot(a->velocity, tangent));
                    b->velocity = v2_mul_s(tangent, v2_dot(b->velocity, tangent));

                    velocity_this_frame = V2_ZERO;
                }
            }
        }

        a->position = v2_add(a->position, v2_mul_s(velocity_this_frame, dt));
    }
}

static void world_render(const GameWorld *world, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
#if 1
    for (s32 i = 0; i < world->entity_count; ++i) {
        entity_render(&world->entities[i], rb, assets, scratch);
    }

#if 0
    render_batch_push_rect(rb, scratch, md_rect, (RGBA32){0, 0.5f, 0.5f, 0.5f}, assets->shader2, 0);

    render_batch_push_rect(rb, scratch, (Rectangle){{0, 0}, {1, 1}}, (RGBA32){1, 0.5f, 0.5f, 0.9f}, assets->shader2, 0);
#endif
#else
    render_batch_push_line(rb, scratch, line_a.start, line_a.end, RGBA32_WHITE, 1.0f, assets->shader2, 0);
    render_batch_push_line(rb, scratch, line_b.start, line_b.end, RGBA32_BLUE, 1.0f, assets->shader2, 0);

    LineIntersection i = line_intersection(line_a, line_b, INTERSECTION_EPSILON);

    if (i.are_intersecting) {
        render_batch_push_circle(rb, scratch, i.intersection_point, RGBA32_RED, 2.0f, assets->shader2, 0);
    }
#endif
}

static void game_update(GameState *game_state, const Input *input, f32 dt)
{
    world_update(&game_state->world, input, dt);
}

static void game_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets)
{
    world_render(&game_state->world, render_cmds, assets, &game_state->frame_arena);
}

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets,
    FrameData frame_data)
{
    la_reset(&game_state->frame_arena);

    game_update(game_state, frame_data.input, frame_data.dt);
    game_render(game_state, render_cmds, assets);
}
