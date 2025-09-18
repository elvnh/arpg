#ifndef TILEMAP_H
#define TILEMAP_H

#define TILEMAP_WIDTH 4
#define TILEMAP_HEIGHT 4
#define TILE_SIZE 64

typedef enum {
    TILE_FLOOR = 0,
    TILE_WALL,
} TileType;

typedef struct {
    TileType tiles[TILEMAP_WIDTH * TILEMAP_HEIGHT];
} Tilemap;

static inline TileType *tilemap_get_tile(Tilemap *tilemap, Vector2i coords)
{
    TileType *result = 0;

    // TODO: IN_RANGE macro
    if ((coords.x >= 0) && (coords.x < TILEMAP_WIDTH) && (coords.y >= 0) && (coords.y < TILEMAP_HEIGHT)) {
        result = &tilemap->tiles[coords.x + (TILEMAP_HEIGHT - 1 - coords.y) * TILEMAP_WIDTH];
    }

    return result;
}


#endif //TILEMAP_H
