#ifndef TILEMAP_H
#define TILEMAP_H

#include "base/linear_arena.h"
#include "base/vector.h"
#include "base/rectangle.h"

#define TILEMAP_MAX_TILES   2048
#define TILE_SIZE           64

typedef enum {
    TILE_FLOOR = 0,
    TILE_WALL,
} TileType;

typedef struct {
    TileType type;
} Tile;

typedef struct TileNode {
    Tile       tile;
    Vector2i   coordinates;
    struct TileNode  *next;
} TileNode;

typedef struct {
    TileNode *tile_nodes[TILEMAP_MAX_TILES]; // NOTE: must be power of 2
    s32 min_x;
    s32 min_y;
    s32 max_x;
    s32 max_y;
} Tilemap;

void   tilemap_insert_tile(Tilemap *tilemap, Vector2i coords, TileType type, LinearArena *arena);
Tile  *tilemap_get_tile(Tilemap *tilemap, Vector2i coords);
Rectangle tilemap_get_bounding_box(const Tilemap *tilemap);

#endif //TILEMAP_H
