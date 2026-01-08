#include "tilemap.h"

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
void tilemap_insert_tile(Tilemap *tilemap, Vector2i coords, TileType type, FreeListArena *arena)
{
    ASSERT(is_pow2(TILEMAP_MAX_TILES));

    ssize index = tilemap_hashed_index(coords);

    TileNode *new_node = fl_alloc_item(arena, TileNode);
    new_node->tile = (Tile){type};
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
