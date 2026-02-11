#ifndef CHUNK_H
#define CHUNK_H

#include "base/rectangle.h"

#define CHUNK_SIZE_IN_TILES 8

struct LinearArena;
struct Tilemap;

typedef struct Chunk {
    int _;
} Chunk;

typedef struct Chunks {
    // TODO: reuse ChunkArray
    Chunk *chunks;
    ssize chunk_count;
    ssize chunk_map_width;
    ssize chunk_map_height;

    // TODO: rename
    s32 x_basis;
    s32 y_basis;
} Chunks;

typedef struct ChunkArray {
    Chunk *chunks;
    ssize chunk_count;
} ChunkArray;

Chunks      create_chunks_for_tilemap(struct Tilemap *tilemap, struct LinearArena *arena);
Chunk      *get_chunk_at_position(Chunks *chunks, Vector2 position);
ChunkArray  get_chunks_in_area(Chunks *chunks, Rectangle area, struct LinearArena *arena);

#endif //CHUNK_H
