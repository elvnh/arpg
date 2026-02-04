#ifndef TILEMAP_H
#define TILEMAP_H

#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/vector.h"
#include "base/rectangle.h"

#define TILEMAP_MAX_TILES   2048
#define TILE_SIZE           64

// TODO: move elsewhere
typedef enum {
    CARDINAL_DIR_WEST,
    CARDINAL_DIR_NORTH,
    CARDINAL_DIR_EAST,
    CARDINAL_DIR_SOUTH,
    CARDINAL_DIR_COUNT,
} CardinalDirection;

// TODO: don't return this from tilemap_get_edge_list, client doesn't need dynamic array
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

void   tilemap_insert_tile(Tilemap *tilemap, Vector2i coords, TileType type, FreeListArena *arena);
Tile  *tilemap_get_tile(Tilemap *tilemap, Vector2i coords);
Rectangle tilemap_get_bounding_box(const Tilemap *tilemap);
EdgePool tilemap_get_edge_list(Tilemap *tilemap, Allocator alloc);

// TODO: move elsewhere
static inline Vector2 cardinal_direction_vector(CardinalDirection dir)
{
    switch (dir) {
        case CARDINAL_DIR_NORTH: return (Vector2)  { 0,   1};
        case CARDINAL_DIR_EAST:  return (Vector2)  { 1,   0};
        case CARDINAL_DIR_SOUTH: return (Vector2)  { 0,  -1};
        case CARDINAL_DIR_WEST:  return (Vector2)  {-1,   0};
        case CARDINAL_DIR_COUNT: break;
    }

    ASSERT(0);
    return (Vector2){0};
}

#endif //TILEMAP_H
