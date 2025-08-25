#ifndef LINEAR_ARENA_H
#define LINEAR_ARENA_H

#include "typedefs.h"
#include "allocator.h"

// TODO: should these take signed size instead of s64?

#define LinearArena_AllocArray(arena, type, count) (type *)(LinearArena_Alloc(arena, count, sizeof(type), AlignOf(type)))
#define LinearArena_AllocItem(arena, type) LinearArena_AllocArray(arena, type, 1)

typedef struct ArenaBlock {
    struct ArenaBlock *next_block;
    s64         used_size;
    s64         capacity;
} ArenaBlock;

typedef struct {
    Allocator parent;

    ArenaBlock *first_block;
    ArenaBlock *top_block;
} LinearArena;

LinearArena LinearArena_Create(Allocator parent, s64 capacity);
void        LinearArena_Destroy(LinearArena *arena);
void       *LinearArena_Alloc(void *context, s64 item_count, s64 item_size, s64 alignment);
void        LinearArena_Free(void *context, void *ptr);
void        LinearArena_Reset(LinearArena *arena);
bool        LinearArena_TryExtend(void *context, void *ptr, ssize old_size, ssize new_size);
Allocator   LinearArena_Allocator(LinearArena *arena);

#endif //LINEAR_ARENA_H
