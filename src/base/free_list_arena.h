#ifndef FREE_LIST_ARENA_H
#define FREE_LIST_ARENA_H

#include "allocator.h"

#define fl_alloc_array(arena, type, count) ((type *)(fl_allocate(arena, count, SIZEOF(type), ALIGNOF(type))))
#define fl_alloc_item(arena, type) fl_alloc_array(arena, type, 1)
#define fl_realloc_array(arena, type, ptr, new_count) ((type *)(fl_reallocate(arena, ptr, new_count, \
                SIZEOF(type), ALIGNOF(type))))

/*
  TODO:
  - Choose between first and best fit
  - Make consistent whether dealing with void * or ssize
  - Total capacity
*/

typedef struct {
    struct FreeListBuffer *head;
    struct FreeListBuffer *tail;

    Allocator parent;
} FreeListArena;

FreeListArena   fl_create(Allocator parent, ssize capacity);
void		fl_destroy(FreeListArena *arena);
void           *fl_allocate(void *context, ssize item_count, ssize item_size, ssize alignment);
void            fl_deallocate(void *context, void *ptr);
void	       *fl_reallocate(void *context, void *ptr, ssize new_count, ssize item_size, ssize alignment);
ssize	        fl_get_memory_usage(FreeListArena *arena);
ssize		fl_get_available_memory(FreeListArena *arena);
Allocator       fl_allocator(FreeListArena *arena);
void            fl_reset(FreeListArena *arena);

#endif //FREE_LIST_ARENA_H
