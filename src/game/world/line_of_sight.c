#include "line_of_sight.h"
#include "base/line.h"
#include "base/maths.h"
#include "base/rectangle.h"
#include "base/utils.h"
#include "base/vector.h"
#include "debug.h"
#include "entity/entity_system.h"
#include "tilemap.h"
#include "world.h"

typedef enum {
    SIDE_NORTH_SOUTH,
    SIDE_WEST_EAST,
} WallSide;

// https://lodev.org/cgtutor/raycasting.html
Vector2 find_first_wall_in_direction(Tilemap *tilemap, Vector2 origin, Vector2 dir)
{
    // The current ray position
    f64 pos_x = origin.x;
    f64 pos_y = origin.y;

    // The current integral map position
    s32 map_x = (s32)pos_x;
    s32 map_y = (s32)pos_y;

    // The current step direction
    s32 step_x = 0;
    s32 step_y = 0;

    // The length from one x or y side to next one
    f64 delta_dist_x = (dir.x == 0.0f) ? 1e30 : fabs(1.0 / (f64)dir.x);
    f64 delta_dist_y = (dir.y == 0.0f) ? 1e30 : fabs(1.0 / (f64)dir.y);

    b32 wall_hit = false;
    WallSide side = 0;

    // The current length of the ray to next x or y side
    f64 side_dist_x = 0.0;
    f64 side_dist_y = 0.0;

    if (dir.x < 0.0f) {
	step_x = -1;
	side_dist_x = (pos_x - map_x) * delta_dist_x;
    } else {
	step_x = 1;
	side_dist_x = (map_x + 1.0 - pos_x) * delta_dist_x;
    }

    if (dir.y < 0) {
	step_y = -1;
	side_dist_y = (pos_y - map_y) * delta_dist_y;
    } else {
	step_y = 1;
	side_dist_y = (map_y + 1.0 - pos_y) * delta_dist_y;
    }

    while (!wall_hit) {
	if (side_dist_x < side_dist_y) {
	    side_dist_x += delta_dist_x;
	    map_x += step_x;
	    side = SIDE_WEST_EAST;
	} else {
	    side_dist_y += delta_dist_y;
	    map_y += step_y;
	    side = SIDE_NORTH_SOUTH;
	}

	Vector2i map_coords = world_to_tile_coords(v2((f32)map_x, (f32)map_y));
	Tile *tile = tilemap_get_tile(tilemap, map_coords);

	if (!tile || (tile->type == TILE_WALL)) {
	    wall_hit = true;
	}
    }

    f32 fraction_x = 0.0f;
    f32 fraction_y = 0.0f;

    // If we hit a vertical line there will be no x fraction, and if we hit a horizontal
    // line there will be no y fraction
    if ((side == SIDE_NORTH_SOUTH) && (dir.x != 0.0f)) {
	fraction_x = fraction((f32)side_dist_x);
    } else if (dir.y != 0.0f) {
	fraction_y = fraction((f32)side_dist_y);
    }

    Vector2 result = {
	(f32)map_x + fraction_x,
	(f32)map_y + fraction_y,
    };

    return result;
}

// TODO: better name
// TODO: instead provide max distance?
Vector2 find_first_wall_on_path(Vector2 origin, Vector2 target, Tilemap *tilemap)
{
    Vector2 direction = v2_norm(v2_sub(target, origin));
    Vector2 first_wall = find_first_wall_in_direction(tilemap, origin, direction);

    Vector2 result = {0};

    if (v2_dist_sq(origin, first_wall) < v2_dist_sq(origin, target)) {
        result = first_wall;
    } else {
        result = target;
    }

    return result;
}

static b32 has_line_of_sight_to_point(Vector2 origin, Vector2 point, Tilemap *tilemap)
{
    // TODO: implement this in a better way by finding first wall but aborting if exceeding distance
    Vector2 dir = v2_sub(point, origin);
    Vector2 intersection = find_first_wall_in_direction(tilemap, origin, dir);

    f32 dist_to_point = v2_dist_sq(origin, point);
    f32 dist_to_intersection = v2_dist_sq(origin, intersection);

    b32 result = (dist_to_point <= dist_to_intersection) ;
    return result;
}

b32 has_line_of_sight_to_entity(PhysicsComponent *self_physics, Entity *other, Tilemap *tilemap)
{
    b32 result = false;

    PhysicsComponent *other_physics = es_get_component(other, PhysicsComponent);

    if (other_physics) {
        // TODO: maybe origin should be center of entity bounds or something
        Vector2 origin = self_physics->position;
        Rectangle other_bounds = world_get_entity_bounding_box(other, other_physics);

        Vector2 other_vertices[] = {
            rect_top_left(other_bounds),
            rect_top_right(other_bounds),
            rect_bottom_right(other_bounds),
            rect_bottom_left(other_bounds),
            rect_center(other_bounds),
        };

        // Check if any of the rectangle corners or the center of the other entity are visible.
        // This isn't exactly correct and may return false even in somme edge cases where the entity is
        // obviously visible, but this should be a good enough approximation for now.
        for (s32 i = 0; i < ARRAY_COUNT(other_vertices); ++i) {
            if (has_line_of_sight_to_point(origin, other_vertices[i], tilemap)) {
                result = true;
                break;
            }
        }
    }


    return result;
}
