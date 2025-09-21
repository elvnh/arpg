#ifndef TILEMAP_H
#define TILEMAP_H

#include "base/linear_arena.h"
#include "base/vector.h"

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
} Tilemap;

void   tilemap_insert_tile(Tilemap *tilemap, Vector2i coords, TileType type, LinearArena *arena);
Tile  *tilemap_get_tile(Tilemap *tilemap, Vector2i coords);

#endif //TILEMAP_H
