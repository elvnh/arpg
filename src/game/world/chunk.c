#include "chunk.h"
#include "base/linear_arena.h"
#include "base/utils.h"
#include "world/tilemap.h"

Chunks create_chunks_for_tilemap(Tilemap *tilemap, LinearArena *arena)
{
    ssize tilemap_width = tilemap->max_x - tilemap->min_x + 1;
    ssize tilemap_height = tilemap->max_y - tilemap->min_y + 1;

    // TODO: function
    ssize chunk_map_width = (ssize)ceilf((f32)tilemap_width / (f32)CHUNK_SIZE_IN_TILES);
    ssize chunk_count = chunk_map_width * (ssize)ceilf((f32)tilemap_height / (f32)CHUNK_SIZE_IN_TILES);

    Chunks result = {0};
    result.chunks = la_allocate_array(arena, Chunk, chunk_count);
    result.chunk_count = chunk_count;
    result.chunk_map_width = chunk_map_width;

    return result;
}

Chunk *get_chunk_at_position(Chunks *chunks, Vector2 position)
{
    ssize chunk_x = (ssize)(position.x / (f32)(CHUNK_SIZE_IN_TILES * TILE_SIZE));
    ssize chunk_y = (ssize)(position.y / (f32)(CHUNK_SIZE_IN_TILES * TILE_SIZE));

    ssize index = chunk_x + chunk_y * chunks->chunk_map_width;

    Chunk *result = 0;

    if (index < chunks->chunk_count) {
        result = &chunks->chunks[index];
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
