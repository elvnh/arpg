#include "chunk.h"
#include "base/linear_arena.h"
#include "base/utils.h"
#include "world/tilemap.h"

Chunks create_chunks_for_tilemap(Tilemap *tilemap, LinearArena *arena)
{
    ssize tilemap_width  = tilemap->max_x - tilemap->min_x + 1;
    ssize tilemap_height = tilemap->max_y - tilemap->min_y + 1;

    s32 chunk_map_width  = (s32)ceilf((f32)tilemap_width  / (f32)CHUNK_SIZE_IN_TILES);
    s32 chunk_map_height = (s32)ceilf((f32)tilemap_height / (f32)CHUNK_SIZE_IN_TILES);
    s32 chunk_count = chunk_map_width * chunk_map_height;

    Chunks result = {0};
    result.chunks = la_allocate_array(arena, Chunk, chunk_count);
    result.chunk_count = chunk_count;
    result.chunk_grid_dims = v2i(chunk_map_width, chunk_map_height);
    result.x_basis = tilemap->min_x;
    result.y_basis = tilemap->min_y;

    return result;
}

Chunk *get_chunk_at_position(Chunks *chunks, Vector2 position)
{
    f32 chunk_pixel_size = (f32)(CHUNK_SIZE_IN_TILES * TILE_SIZE);
    ssize chunk_x = (ssize)floorf((position.x - (f32)chunks->x_basis * (f32)TILE_SIZE) / chunk_pixel_size);
    ssize chunk_y = (ssize)floorf((position.y - (f32)chunks->y_basis * (f32)TILE_SIZE) / chunk_pixel_size);

    Chunk *result = 0;

    b32 in_bounds = (chunk_x >= 0) && (chunk_y >= 0)
        && (chunk_x < chunks->chunk_grid_dims.x)
        && (chunk_y < chunks->chunk_grid_dims.y);

    if (in_bounds) {
        ssize index = chunk_x + chunk_y * chunks->chunk_grid_dims.x;

        if (index < chunks->chunk_count) {
            result = &chunks->chunks[index];
        }
    }

    return result;
}

ChunkArray get_chunks_in_area(Chunks *chunks, Rectangle area, LinearArena *arena)
{
    (void)chunks;
    (void)area;
    (void)arena;
    UNIMPLEMENTED;

    ChunkArray result = {0};

    return result;
}
