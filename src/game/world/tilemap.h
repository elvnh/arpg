#ifndef TILEMAP_H
#define TILEMAP_H

#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/vector.h"
#include "base/rectangle.h"
#include "base/direction.h"

#define TILEMAP_MAX_TILES   2048
#define TILE_SIZE           64

/*
  TODO:
  - Don't return dynamic array from tilemap_get_edge_list
  - Prebake static lights into texture
 */

typedef struct {
    Line line;
    CardinalDirection direction;
} EdgeLine;

typedef struct {
    EdgeLine *items;
    ssize count;
    ssize capacity;
} EdgePool;

typedef ssize WallEdgeID;

typedef struct {
    b32 exists;
    WallEdgeID id;
} WallEdge;

typedef enum {
    TILE_FLOOR = 0,
    TILE_WALL,
} TileType;

typedef struct {
    TileType type;
    WallEdge edges[CARDINAL_DIR_COUNT];
} Tile;

typedef struct TileNode {
    Tile       tile;
    Vector2i   coordinates;
    struct TileNode  *next_in_hash;
} TileNode;

typedef struct Tilemap {
    TileNode *tile_nodes[TILEMAP_MAX_TILES]; // NOTE: must be power of 2
    s32 min_x;
    s32 min_y;
    s32 max_x;
    s32 max_y;
} Tilemap;

void   tilemap_initialize(Tilemap *tilemap);
void   tilemap_insert_tile(Tilemap *tilemap, Vector2i coords, TileType type, LinearArena *arena);
Tile  *tilemap_get_tile(Tilemap *tilemap, Vector2i coords);
Rectangle tilemap_get_bounding_box(const Tilemap *tilemap);
EdgePool tilemap_get_edge_list(Tilemap *tilemap, Allocator alloc);

#endif //TILEMAP_H
