#include "tilemap.h"
#include "base/line.h"
#include "base/dynamic_array.h"
#include "base/vector.h"
#include "world.h"

static inline u64 hash_v2i(Vector2i v)
{
    u64 result = ((u64)v.x << (u64)32) | (u64)v.y;

    return result;
}

static inline ssize tilemap_hashed_index(Vector2i coords)
{
    u64 hash = hash_v2i(coords);
    ssize index = hash & (TILEMAP_MAX_TILES - 1);

    return index;
}

// TODO: allocating tiles from a freelist arena is kind of inefficient
void tilemap_insert_tile(Tilemap *tilemap, Vector2i coords, TileType type, LinearArena *arena)
{
    ASSERT(is_pow2(TILEMAP_MAX_TILES));

    ssize index = tilemap_hashed_index(coords);

    TileNode *new_node = la_allocate_item(arena, TileNode);
    new_node->tile.type = type;
    new_node->coordinates = coords;

    TileNode *curr_node = tilemap->tile_nodes[index];

    if (curr_node) {
        ASSERT(!v2i_eq(curr_node->coordinates, coords));

        while (curr_node->next_in_hash) {
            ASSERT(!v2i_eq(curr_node->coordinates, coords));

            curr_node = curr_node->next_in_hash;
        }

        curr_node->next_in_hash = new_node;

    } else {
        tilemap->tile_nodes[index] = new_node;
    }

    tilemap->min_x = MIN(tilemap->min_x, coords.x);
    tilemap->min_y = MIN(tilemap->min_y, coords.y);
    tilemap->max_x = MAX(tilemap->max_x, coords.x);
    tilemap->max_y = MAX(tilemap->max_y, coords.y);
}

Tile *tilemap_get_tile(Tilemap *tilemap, Vector2i coords)
{
    ssize index = tilemap_hashed_index(coords);

    TileNode *node = tilemap->tile_nodes[index];

    while (node) {
        if (v2i_eq(node->coordinates, coords)) {
            break;
        }

        node = node->next_in_hash;
    }

    if (node) {
        return &node->tile;
    }

    return 0;
}

Rectangle tilemap_get_bounding_box(const Tilemap *tilemap)
{
    Rectangle result = {
	{(f32)(tilemap->min_x * TILE_SIZE), (f32)(tilemap->min_y * TILE_SIZE)},
	{(f32)((tilemap->max_x - tilemap->min_x + 1) * TILE_SIZE),
	 (f32)((tilemap->max_y - tilemap->min_y + 1) * TILE_SIZE)}
    };

    return result;
}

static Tile *get_tile_neighbour(Tilemap *tilemap, Vector2i tile_coords, CardinalDirection dir)
{
    Vector2 v = cardinal_direction_vector(dir);
    Vector2i neighbour_coords = v2i_add(tile_coords, v2_to_v2i(v));

    Tile *result = tilemap_get_tile(tilemap, neighbour_coords);
    return result;
}

static Vector2 get_wall_line_segment_begin(Vector2 tile_world_coords, CardinalDirection edge_dir)
{
    Vector2 result = {0};

    switch (edge_dir) {
        case CARDINAL_DIR_NORTH: {
        case CARDINAL_DIR_WEST:
            result = v2_add(tile_world_coords, v2(0, TILE_SIZE));
        } break;

        case CARDINAL_DIR_EAST: {
            result = v2_add(tile_world_coords, v2(TILE_SIZE, TILE_SIZE));
        } break;

        case CARDINAL_DIR_SOUTH: {
            ASSERT(0);
        } break;

        default: ASSERT(0);
    }

    return result;
}

static Vector2 wall_line_direction_vector(CardinalDirection edge_dir)
{
    switch (edge_dir) {
        case CARDINAL_DIR_NORTH:
        case CARDINAL_DIR_SOUTH:
            return (Vector2)  {1, 0};

        case CARDINAL_DIR_EAST:
        case CARDINAL_DIR_WEST:
            return (Vector2)  {0, -1};

        default:
            ASSERT(0);
            return V2_ZERO;
    }
}

static Vector2 get_wall_line_segment_end(Vector2 line_begin, CardinalDirection edge_dir)
{
    Vector2 wall_line_dir = wall_line_direction_vector(edge_dir);
    Vector2 wall_line_length = v2_mul_s(wall_line_dir, (f32)TILE_SIZE);

    Vector2 result = v2_add(line_begin, wall_line_length);

    return result;
}

static CardinalDirection get_connectible_neighbour_direction(CardinalDirection edge_dir)
{
    switch (edge_dir) {
        case CARDINAL_DIR_WEST:
        case CARDINAL_DIR_EAST:
            return CARDINAL_DIR_NORTH;

        case CARDINAL_DIR_NORTH:
            return CARDINAL_DIR_WEST;

        case CARDINAL_DIR_SOUTH:
            ASSERT(0 && "Southern edges aren't created anymore");
            break;

        case CARDINAL_DIR_COUNT:
            ASSERT(0);
            break;
    }

    ASSERT(0);
    return 0;
}

static void set_wall_edge_id(Tile *tile, CardinalDirection edge_dir, WallEdgeID edge_id)
{
    ASSERT(tile->type == TILE_WALL && "Only walls can have edges");
    ASSERT(edge_id >= 0);
    ASSERT(!tile->edges[edge_dir].exists && "Edge shouldn't already exist");

    tile->edges[edge_dir].id = edge_id;
    tile->edges[edge_dir].exists = true;
}

static void try_create_wall_edge_on_side(Tilemap *tilemap, Tile *tile, Vector2i tile_coords,
    CardinalDirection edge_dir, EdgePool *edge_pool, Allocator alloc)
{
    ASSERT(tile);
    ASSERT(tile->type == TILE_WALL);

    Vector2 tile_world_coords = tile_to_world_coords(tile_coords);

    Tile *edge_neighbour = get_tile_neighbour(tilemap, tile_coords, edge_dir);

    if (!edge_neighbour || (edge_neighbour->type != TILE_WALL)) {
        CardinalDirection neighbour_dir = get_connectible_neighbour_direction(edge_dir);
        Tile *connected_neighbour = get_tile_neighbour(tilemap, tile_coords, neighbour_dir);

        Vector2 wall_line_begin = get_wall_line_segment_begin(tile_world_coords, edge_dir);
        Vector2 wall_line_end = get_wall_line_segment_end(wall_line_begin, edge_dir);

        WallEdgeID edge_id = -1;

        b32 can_extend_neighbouring_edge = connected_neighbour
            && connected_neighbour->edges[edge_dir].exists;

        if (can_extend_neighbouring_edge) {
            // Our neighbour has already created an edge, extend it
            ASSERT((connected_neighbour->type == TILE_WALL)
                && "If this isn't a wall, the edge shouldn't be marked as existing");

            edge_id = connected_neighbour->edges[edge_dir].id;

            Line *existing_line = &edge_pool->items[edge_id].line;
            existing_line->end = wall_line_end;
        } else {
            // No edge exists, create a new one
            edge_id = edge_pool->count;

            Line new_line = {wall_line_begin, wall_line_end};
            EdgeLine edge = {new_line, edge_dir};

            // TODO: return pointer to element from da_push
            da_push(edge_pool, edge, alloc);
        }

        set_wall_edge_id(tile, edge_dir, edge_id);
    }
}

EdgePool tilemap_get_edge_list(Tilemap *tilemap, Allocator alloc)
{
    EdgePool edge_pool = {0};
    da_init(&edge_pool, 128, alloc);

    // NOTE: we go through from TOP to BOTTOM, LEFT to RIGHT
    for (s32 y = tilemap->max_y; y >= tilemap->min_y; --y) {
        for (s32 x = tilemap->min_x; x <= tilemap->max_x; ++x) {
            Vector2i tile_coords = {x, y};
            Tile *tile = tilemap_get_tile(tilemap, tile_coords);

            if (tile && (tile->type == TILE_WALL)) {
                // TODO: instead cache results
                memset(tile->edges, 0, (usize)ARRAY_COUNT(tile->edges) * sizeof(*tile->edges));

                // TODO: not needed
                try_create_wall_edge_on_side(tilemap, tile, tile_coords,
                    CARDINAL_DIR_NORTH, &edge_pool, alloc);
                try_create_wall_edge_on_side(tilemap, tile, tile_coords,
                    CARDINAL_DIR_WEST, &edge_pool, alloc);
                try_create_wall_edge_on_side(tilemap, tile, tile_coords,
                    CARDINAL_DIR_EAST, &edge_pool, alloc);
            }
        }
    }

    return edge_pool;
}
