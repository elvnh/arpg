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

void tilemap_insert_tile(Tilemap *tilemap, Vector2i coords, TileType type, LinearArena *arena)
{
    ASSERT(is_pow2(TILEMAP_MAX_TILES));

    ssize index = tilemap_hashed_index(coords);

    TileNode *new_node = la_allocate_item(arena, TileNode);
    new_node->tile = (Tile){type};
    new_node->coordinates = coords;

    TileNode *curr_node = tilemap->tile_nodes[index];

    if (curr_node) {
        ASSERT(!v2i_eq(curr_node->coordinates, coords));

        while (curr_node->next) {
            ASSERT(!v2i_eq(curr_node->coordinates, coords));

            curr_node = curr_node->next;
        }

        curr_node->next = new_node;

    } else {
        tilemap->tile_nodes[index] = new_node;
    }
}

Tile *tilemap_get_tile(Tilemap *tilemap, Vector2i coords)
{
    ssize index = tilemap_hashed_index(coords);

    TileNode *node = tilemap->tile_nodes[index];

    while (node) {
        if (v2i_eq(node->coordinates, coords)) {
            break;
        }

        node = node->next;
    }

    if (node) {
        return &node->tile;
    }

    return 0;
}
