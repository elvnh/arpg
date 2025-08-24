#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "utils.h"

#define AllocArray(alloc, type, count) (type *)(Allocate(alloc, count, sizeof(type), AlignOf(type)))
#define AllocItem(alloc, type) AllocArray(alloc, type, 1)

typedef void *(*AllocFunction)(void*, s64, s64, s64);
typedef void  (*FreeFunction)(void*, void*);

typedef struct {
    AllocFunction alloc;
    FreeFunction  dealloc;

    void *context;
} Allocator;

static inline void *Allocate(Allocator allocator, s64 item_count, s64 item_size, s64 alignment)
{
    return allocator.alloc(allocator.context, item_count, item_size, alignment);
}

#endif //ALLOCATOR_H
