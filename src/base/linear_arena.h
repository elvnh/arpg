#ifndef LINEAR_ARENA_H
#define LINEAR_ARENA_H

#include "typedefs.h"
#include "allocator.h"

#define la_allocate_array(arena, type, count) (type *)((la_allocate(arena, count, sizeof(type), ALIGNOF(type))))
#define la_allocate_item(arena, type) la_allocate_array(arena, type, 1)
#define la_copy_array(arena, ptr, count) la_copy_allocation((arena), (ptr), (count), sizeof(*(ptr)), ALIGNOF(*(ptr)))

typedef struct LinearArena {
    Allocator           parent;
    struct ArenaBlock  *first_block;
    struct ArenaBlock  *top_block;
    ssize               offset_into_top_block;
} LinearArena;

LinearArena  la_create(Allocator parent, ssize capacity);
void         la_destroy(LinearArena *arena);
void        *la_allocate(void *context, ssize item_count, ssize item_size, ssize alignment);
void         la_reset(LinearArena *arena);
Allocator    la_allocator(LinearArena *arena);
ssize        la_get_memory_usage(LinearArena *arena);
void         la_pop_to(LinearArena *arena, void *ptr);
void        *la_copy_allocation(void *context, void *arr, ssize item_count, ssize item_size, ssize alignment);

#endif //LINEAR_ARENA_H
