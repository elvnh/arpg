#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdlib.h>

#include "utils.h"

// Helper macros to make it easier to call Allocator function pointers
// TODO: allocate should take item count and item size
#define allocate(a, byte_count) ((void *)allocate_array((a), byte, byte_count))
#define allocate_item(a, type) allocate_array((a), type, 1)
#define allocate_array(a, type, count) (type *)((a).alloc(a.context, (count), sizeof(type), ALIGNOF(type)))
#define allocate_aligned(a, count, item_size, alignment) (void *)(a).alloc((a).context, (count), (item_size), (alignment))
#define deallocate(a, ptr) (a).dealloc((a).context, (ptr))

#define default_allocator (Allocator){ .context = 0, .alloc = default_allocate, \
            .dealloc = default_deallocate }

typedef void *(*AllocateFunction)(void*, ssize, ssize, ssize);
typedef void  (*DeallocateFunction)(void*, void*);

typedef struct {
    AllocateFunction     alloc;
    DeallocateFunction   dealloc;
    void                *context;
} Allocator;

void  *default_allocate(void *ctx, ssize item_count, ssize item_size, ssize alignment);
void   default_deallocate(void *ctx, void *ptr);

void  *stub_allocate(void *ctx, ssize item_count, ssize item_size, ssize alignment);
void   stub_deallocate(void *ctx, void *ptr);

#endif //ALLOCATOR_H
