#define _GNU_SOURCE
#include <stdlib.h>

#include "light.h"
#include "base/linear_arena.h"
#include "base/line.h"
#include "base/list.h"
#include "base/polygon.h"
#include "base/vector.h"
#include "tilemap.h"
#include "line_of_sight.h"
#include "world.h"
#include "base/utils.h"
#include "renderer/frontend/render_batch.h"
#include "asset_table.h"
#include <math.h>

#define RAYS_PER_CORNER 3
#define RAY_OFFSET_FROM_CORNER 0.00001f

#define LIGHT_SHADER_ORIGIN_UNIFORM_NAME str_lit("u_light_origin")
#define LIGHT_SHADER_RADIUS_UNIFORM_NAME str_lit("u_light_radius")

typedef struct {
    Vector2 intersection_point;
    f32 ray_angle;
} RayHit;

typedef struct {
    RayHit *items;
    ssize count;
} RayHits;

// TODO: all these functions should be in the line of sight file
int sort_points_cmp(const void *a, const void *b, void *data)
{
    (void)data;

    RayHit hit_a = *(RayHit *)a;
    RayHit hit_b = *(RayHit *)b;

    f32 angle_a = hit_a.ray_angle;
    f32 angle_b = hit_b.ray_angle;

    if (angle_a < angle_b) {
        return -1;
    } else if (angle_a > angle_b) {
        return 1;
    }

    return 0;
}

static void cast_rays_towards_corner(RayHits *hit_array, Vector2 origin, Vector2 target, EdgePool *edges)
{
    Vector2 target_direction = v2_norm(v2_sub(target, origin));
    f32 target_angle = atan2f(target_direction.y, target_direction.x);

    for (ssize i = 0; i < RAYS_PER_CORNER; ++i) {
	f32 angle_offset = -RAY_OFFSET_FROM_CORNER + RAY_OFFSET_FROM_CORNER * (f32)i;
	f32 ray_angle = target_angle + angle_offset;

	Vector2 ray_direction = {cosf(ray_angle), sinf(ray_angle)};

        Vector2 closest_hit = {0};
        f32 closest_hit_dist_sq = INFINITY;

        for (ssize j = 0; j < edges->count; ++j) {
            EdgeLine edge = edges->items[j];

            b32 should_raycast = true;

            // For vertical wall edges, we want them to be lit if we're visually in front of them
            if (edge.direction == CARDINAL_DIR_WEST) {
                // Raycast against west wall if either above it, or in front of it and to the right
                should_raycast = (origin.y >= edge.line.end.y) || (origin.x >= edge.line.end.x);
            } else if (edge.direction == CARDINAL_DIR_EAST) {
                // Raycast against east wall if either above it, or in front of it and to the left
                should_raycast = (origin.y >= edge.line.end.y) || (origin.x <= edge.line.end.x);
            }

            if (should_raycast) {
                LineIntersection intersection = ray_vs_line_intersection(origin, ray_direction, edge.line);

                if (intersection.are_intersecting) {
                    f32 dist_sq = v2_dist_sq(origin, intersection.intersection_point);

                    if (dist_sq < closest_hit_dist_sq) {
                        closest_hit_dist_sq = dist_sq;
                        closest_hit = intersection.intersection_point;
                    }
                }
            }
        }

        if (!isinf(closest_hit_dist_sq)) {
            RayHit hit = {closest_hit, ray_angle};
            hit_array->items[hit_array->count++] = hit;
        }
    }
}

TriangleFan get_visibility_polygon(Vector2 origin, Tilemap *tilemap, LinearArena *arena)
{
    Allocator alloc = la_allocator(arena);
    EdgePool edge_pool = tilemap_get_edge_list(tilemap, alloc);

    RayHits ray_hits = {0};
    ray_hits.items = la_allocate_array(arena, RayHit, edge_pool.count * 2 * RAYS_PER_CORNER);

    for (ssize i = 0; i < edge_pool.count; ++i) {
	 EdgeLine edge = edge_pool.items[i];

	cast_rays_towards_corner(&ray_hits, origin, edge.line.start, &edge_pool);
	cast_rays_towards_corner(&ray_hits, origin, edge.line.end, &edge_pool);
    }

    // Sort the ray hits in clockwise order around player to allow connecting them in triangle fan
    qsort_r(ray_hits.items, (usize)ray_hits.count, sizeof(*ray_hits.items), sort_points_cmp, 0);

    TriangleFan result = {0};
    result.center = origin;
    result.items = la_allocate_array(arena, TriangleFanElement, ray_hits.count);

    for (ssize i = 0; i < ray_hits.count; ++i) {
	RayHit curr = ray_hits.items[i];
	RayHit next = ray_hits.items[(i + 1) % ray_hits.count];

        TriangleFanElement elem = {curr.intersection_point, next.intersection_point};
	result.items[result.count++] = elem;
    }

    return result;
}

void render_light_source(struct World *world, struct RenderBatch *rb, Vector2 origin, LightSource light,
    f32 intensity, struct LinearArena *arena)
{
    RenderEntry *entry = 0;

    ASSERT(light.color.a > 0.0f);

    RGBA32 color = light.color;
    color.a *= intensity;

    if (light.fading_out) {
	if (light.fade_duration == 0.0f) {
	    color.a = 0.0f;
	} else {
	    color.a = light.color.a * (1.0f - MIN(light.time_elapsed / light.fade_duration, 1.0f));
	}
    }

    color.a = MAX(0.0f, color.a);

    switch (light.kind) {
        case LIGHT_REGULAR: {
            entry = draw_circle(rb, arena, origin, color, light.radius,
                get_asset_table()->light_shader, RENDER_LAYER_OVERLAY);
        } break;

        case LIGHT_RAYCASTED: {
            TriangleFan fan = get_visibility_polygon(origin, &world->tilemap, arena);
            entry = draw_triangle_fan(rb, arena, fan, color,
                get_asset_table()->light_shader, RENDER_LAYER_OVERLAY);
        } break;

        INVALID_DEFAULT_CASE;
    }

    Vector4 extended_pos = {origin.x, origin.y, 0.0f, 0.0f};
    re_set_uniform_vec4(entry, arena, LIGHT_SHADER_ORIGIN_UNIFORM_NAME, extended_pos);

    re_set_uniform_float(entry, arena, LIGHT_SHADER_RADIUS_UNIFORM_NAME, light.radius);
}
