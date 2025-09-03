#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdlib.h>

#include "utils.h"

// Helper macros to make it easier to call Allocator function pointers
#define allocate(a, bytes) (void *)allocate_array((a), byte, (bytes))
#define allocate_item(a, type) allocate_array((a), type, 1)
#define allocate_array(a, type, count) (type *)((a).alloc(a.context, (count), sizeof(type), ALIGNOF(type)))
#define allocate_aligned(a, size, alignment) (void *)(a).alloc((a).context, (size), 1, (alignment))
#define deallocate(a, ptr) (a).dealloc((a).context, (ptr))
#define try_resize_allocation(a, ptr, old_size, new_size) a.try_resize(a.context, ptr, old_size, new_size)

#define default_allocator (Allocator){ .alloc = default_allocate, \
            .dealloc = default_deallocate, .try_resize = default_try_resize }

typedef void *(*AllocateFunction)(void*, ssize, ssize, ssize);
typedef void  (*DeallocateFunction)(void*, void*);
typedef bool  (*TryResizeFunction)(void*, void*, ssize, ssize);

typedef struct {
    AllocateFunction    alloc;
    DeallocateFunction  dealloc;
    TryResizeFunction   try_resize;
    void               *context;
} Allocator;

void  *default_allocate(void *ctx, ssize item_count, ssize item_size, ssize alignment);
void   default_deallocate(void *ctx, void *ptr);
bool   default_try_resize(void *ctx, void *ptr, ssize old_size, ssize new_size);

void  *stub_allocate(void *ctx, ssize item_count, ssize item_size, ssize alignment);
void   stub_deallocate(void *ctx, void *ptr);
bool   stub_try_resize(void *ctx, void *ptr, ssize old_size, ssize new_size);

#endif //ALLOCATOR_H
