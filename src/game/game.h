#ifndef GAME_H
#define GAME_H

#include "base/matrix.h"
#include "renderer/render_batch.h"
#include "camera.h"
#include "input.h"

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 size;
    RGBA32 color;
} Entity;

typedef enum {
    TILE_NULL,
    TILE_FLOOR,
    TILE_WALL,
} TileType;

#define TILEMAP_WIDTH 4
#define TILEMAP_HEIGHT 4
#define TILE_SIZE 64

typedef struct {
    TileType tiles[TILEMAP_WIDTH * TILEMAP_HEIGHT];
} Tilemap;

static inline TileType *tilemap_get_tile(Tilemap *tilemap, s32 x, s32 y)
{
    TileType *result = 0;

    if ((x >= 0) && (x < TILEMAP_WIDTH) && (y >= 0) && (y < TILEMAP_HEIGHT)) {
        TileType *tile = &tilemap->tiles[x + (TILEMAP_HEIGHT - 1 - y) * TILEMAP_WIDTH];

        if (*tile != TILE_NULL) {
            result = tile;
        }
    }

    return result;
}

typedef struct {
    Entity entities[32];
    s32  entity_count;
    Camera camera;
    Tilemap tilemap;
} GameWorld;

typedef struct {
    GameWorld world;
} GameState;

typedef struct {
    f32 dt;
    const Input *input;
    s32 window_width;
    s32 window_height;
} FrameData;

void game_update_and_render(GameState *game_state, RenderBatch *render_cmds, const AssetList *assets,
    FrameData frame_data, LinearArena *frame_arena);

#endif //GAME_H
