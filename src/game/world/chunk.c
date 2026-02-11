#include "chunk.h"
#include "base/linear_arena.h"
#include "base/utils.h"
#include "world/tilemap.h"
#include "world/world.h"

Chunks create_chunks_for_tilemap(Tilemap *tilemap, LinearArena *arena)
{
    s32 tilemap_width  = tilemap->max_x - tilemap->min_x + 1;
    s32 tilemap_height = tilemap->max_y - tilemap->min_y + 1;

    s32 chunk_map_width  = (s32)ceilf((f32)tilemap_width  / (f32)CHUNK_SIZE_IN_TILES);
    s32 chunk_map_height = (s32)ceilf((f32)tilemap_height / (f32)CHUNK_SIZE_IN_TILES);
    s32 chunk_count = chunk_map_width * chunk_map_height;

    Chunks result = {0};
    result.chunks = la_allocate_array(arena, Chunk, chunk_count);
    result.chunk_count = chunk_count;
    result.chunk_grid_dims = v2i(chunk_map_width, chunk_map_height);
    result.chunk_grid_base_tile_coords = v2i(tilemap->min_x, tilemap->min_y);

    return result;
}

Chunk *get_chunk_at_position(Chunks *chunks, Vector2 position)
{
    f32 chunk_world_size = (f32)(CHUNK_SIZE_IN_TILES * TILE_SIZE);

    Vector2 chunks_base_world_coords = tile_to_world_coords(chunks->chunk_grid_base_tile_coords);

    s32 chunk_x = (s32)floorf((position.x - chunks_base_world_coords.x) / chunk_world_size);
    s32 chunk_y = (s32)floorf((position.y - chunks_base_world_coords.y) / chunk_world_size);

    Chunk *result = 0;

    b32 in_bounds = (chunk_x >= 0) && (chunk_y >= 0)
        && (chunk_x < chunks->chunk_grid_dims.x)
        && (chunk_y < chunks->chunk_grid_dims.y);

    if (in_bounds) {
        ssize index = chunk_x + chunk_y * chunks->chunk_grid_dims.x;
        ASSERT(index < chunks->chunk_count);

        result = &chunks->chunks[index];
    }

    return result;
}

ChunkPtrArray get_chunks_in_area(Chunks *chunks, Rectangle area, LinearArena *arena)
{
    ChunkPtrArray result = {0};

    f32 chunk_world_size = (f32)(CHUNK_SIZE_IN_TILES * TILE_SIZE);

    s32 begin_x = (s32)area.position.x;
    s32 begin_y = (s32)area.position.y;
    s32 end_x = begin_x + (s32)ceilf(area.size.x / chunk_world_size);
    s32 end_y = begin_y + (s32)ceilf(area.size.y / chunk_world_size);

    ssize horizontal_chunk_count = end_x - begin_x + 1;
    ssize vertical_chunk_count = end_y - begin_y + 1;
    ssize total_chunk_count = horizontal_chunk_count * vertical_chunk_count;

    result.chunks = la_allocate_array(arena, Chunk *, total_chunk_count);

    for (ssize x = 0; x < horizontal_chunk_count; ++x) {
        for (ssize y = 0; y < vertical_chunk_count; ++y) {
            Vector2 coords = {
                area.position.x + (f32)x * chunk_world_size,
                area.position.y + (f32)y * chunk_world_size
            };

            ASSERT(result.count < total_chunk_count);

            Chunk *chunk = get_chunk_at_position(chunks, coords);
            result.chunks[result.count++] = chunk;
        }
    }


    return result;
}
