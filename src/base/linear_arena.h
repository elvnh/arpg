#ifndef LINEAR_ARENA_H
#define LINEAR_ARENA_H

#include "typedefs.h"
#include "allocator.h"

#define arena_allocate_array(arena, type, count) (type *)(arena_allocate(arena, count, sizeof(type), ALIGNOF(type)))
#define arena_allocate_item(arena, type) arena_allocate_array(arena, type, 1)

typedef struct {
    Allocator           parent;
    struct ArenaBlock  *first_block;
    struct ArenaBlock  *top_block;
    ssize               offset_into_top_block;
} LinearArena;

LinearArena  arena_create(Allocator parent, ssize capacity);
void         arena_destroy(LinearArena *arena);
void        *arena_allocate(void *context, ssize item_count, ssize item_size, ssize alignment);
void         arena_reset(LinearArena *arena);
bool         arena_try_resize(void *context, void *ptr, ssize old_size, ssize new_size);
Allocator    arena_create_allocator(LinearArena *arena);
ssize        arena_get_memory_usage(LinearArena *arena);

#endif //LINEAR_ARENA_H
