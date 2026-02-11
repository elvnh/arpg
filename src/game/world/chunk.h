#ifndef CHUNK_H
#define CHUNK_H

#include "base/rectangle.h"

#define CHUNK_SIZE_IN_TILES 8

struct LinearArena;
struct Tilemap;

typedef struct Chunk {
    // NOTE: the chunks empty for now
    int _;
} Chunk;

typedef struct Chunks {
    Chunk      *chunks;
    ssize       chunk_count;
    Vector2i    chunk_grid_dims;
    Vector2i    chunk_grid_base_tile_coords;
} Chunks;

typedef struct ChunkPtrArray {
    Chunk   **chunks;
    ssize     count;
} ChunkPtrArray;

Chunks           create_chunks_for_tilemap(struct Tilemap *tilemap, struct LinearArena *arena);
Chunk           *get_chunk_at_position(Chunks *chunks, Vector2 position);
ChunkPtrArray    get_chunks_in_area(Chunks *chunks, Rectangle area, struct LinearArena *arena);

#endif //CHUNK_H
