#ifndef LINEAR_ARENA_H
#define LINEAR_ARENA_H

#include "typedefs.h"
#include "allocator.h"

#define LinearArena_AllocArray(arena, type, count) (type *)(LinearArena_Alloc(arena, count, sizeof(type), AlignOf(type)))
#define LinearArena_AllocItem(arena, type) LinearArena_AllocArray(arena, type, 1)

typedef struct {
    Allocator           parent;
    struct ArenaBlock  *first_block;
    struct ArenaBlock  *top_block;
    ssize               offset_into_top_block;
} LinearArena;

LinearArena LinearArena_Create(Allocator parent, ssize capacity);
void        LinearArena_Destroy(LinearArena *arena);
void       *LinearArena_Alloc(void *context, ssize item_count, ssize item_size, ssize alignment);
void        LinearArena_Reset(LinearArena *arena);
bool        LinearArena_TryExtend(void *context, void *ptr, ssize old_size, ssize new_size);
Allocator   LinearArena_Allocator(LinearArena *arena);

#endif //LINEAR_ARENA_H
