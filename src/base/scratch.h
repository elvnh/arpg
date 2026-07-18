#ifndef SCRATCH_H
#define SCRATCH_H

#include "typedefs.h"

struct LinearArena;
struct ArenaBlock;

typedef struct {
    struct LinearArena *arena;
    struct ArenaBlock *start_top_block;
    ssize start_offset_into_top_block;
} TempArena;

TempArena temp_arena_begin(void);
void temp_arena_end(TempArena scratch);

#endif // SCRATCH_H
