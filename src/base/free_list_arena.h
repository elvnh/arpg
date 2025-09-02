#ifndef FREE_LIST_ARENA_H
#define FREE_LIST_ARENA_H

#include "base/allocator.h"
#include "base/list.h"
#include "typedefs.h"

/*
  TODO:
  - Choose between first and best fit
  - Dynamic growth
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
ssize	        fl_get_memory_usage(FreeListArena *arena);
ssize		fl_get_available_memory(FreeListArena *arena);

#endif //FREE_LIST_ARENA_H
