#ifndef FREE_LIST_ARENA_H
#define FREE_LIST_ARENA_H

#include "allocator.h"

/*
  TODO:
  - Choose between first and best fit
  - Helper macros
  - Make consistent whether dealing with void * or ssize
  - Should realloc take item count and item size?
*/

/*
  allocate(ctx, n, size, align) -> void*
  deallocate(ctx, ptr) -> void
  reallocate(ctx, ptr, new_count, size, align) -> void*

  ingen try resize? kan bytas ut mot realloc
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
//bool		fl_try_resize_allocation(FreeListArena *arena, void *ptr, ssize old_size, ssize new_size);
ssize	        fl_get_memory_usage(FreeListArena *arena);
ssize		fl_get_available_memory(FreeListArena *arena);

#endif //FREE_LIST_ARENA_H
