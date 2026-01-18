#include "line_of_sight.h"
#include "base/maths.h"
#include "debug.h"
#include "tilemap.h"
#include "world.h"

typedef enum {
    SIDE_NULL = 0,
    SIDE_NORTH_SOUTH,
    SIDE_WEST_EAST,
} WallSide;

Vector2 find_first_wall_along_path(Tilemap *tilemap, Vector2 origin, Vector2 dir)
{
    // The current ray position
    f64 pos_x = origin.x;
    f64 pos_y = origin.y;

    // The current integral map position
    s32 map_x = (s32)pos_x;
    s32 map_y = (s32)pos_y;

    // The length from one x or y side to next one
    f64 delta_dist_x = (dir.x == 0.0f) ? 1e30 : fabs(1.0 / (f64)dir.x);
    f64 delta_dist_y = (dir.y == 0.0f) ? 1e30 : fabs(1.0 / (f64)dir.y);

    // The current step direction
    s32 step_x = 0;
    s32 step_y = 0;

    b32 wall_hit = false;
    WallSide side = SIDE_NULL;

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
	    side = SIDE_WEST_EAST; // correct?
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

    ASSERT(side != SIDE_NULL);

    // TODO: function for getting fractions
    f32 fraction_x = 0.0f;
    f32 fraction_y = 0.0f;

    // If we hit a vertical line there will be no x fraction, and if we hit a horizontal
    // line there will be no y fraction
    if (side == SIDE_NORTH_SOUTH) {
	fraction_x = (f32)(side_dist_x - (s32)side_dist_x);
    } else {
	fraction_y = (f32)(side_dist_y - (s32)side_dist_y);
    }

    Vector2 result = {
	(f32)map_x + fraction_x,
	(f32)map_y + fraction_y,
    };

    return result;
}
