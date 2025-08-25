#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdlib.h>

#include "utils.h"

#define AllocArray(alloc, type, count) (type *)(Allocate(alloc, count, sizeof(type), AlignOf(type)))
#define AllocItem(alloc, type) AllocArray(alloc, type, 1)
#define Alloc(alloc, bytes) (void *)AllocArray(alloc, byte, bytes)
#define AllocAligned(alloc, size, alignment) (void *)Allocate(alloc, size, 1, alignment)

// TODO: make into const global variable
#define DefaultAllocator (Allocator){ .alloc = DefaultAllocate, .dealloc = DefaultFree, .try_extend = DefaultTryExtend }

typedef void *(*AllocFunction)(void*, ssize, ssize, ssize);
typedef void  (*FreeFunction)(void*, void*);
typedef bool  (*TryExtendFunction)(void*, void*, ssize, ssize);

// TODO: should these be allowed to be null?
typedef struct {
    AllocFunction     alloc;
    FreeFunction      dealloc;
    TryExtendFunction try_extend;

    void *context;
} Allocator;

static inline void *Allocate(Allocator allocator, ssize item_count, ssize item_size, ssize alignment)
{
    return allocator.alloc(allocator.context, item_count, item_size, alignment);
}

static inline void Free(Allocator allocator, void *ptr)
{
    allocator.dealloc(allocator.context, ptr);
}

static inline bool TryExtendAllocation(Allocator allocator, void *ptr, ssize old_size, ssize new_size)
{
    return allocator.try_extend(allocator.context, ptr, old_size, new_size);
}

void *DefaultAllocate(void *ctx, ssize item_count, ssize item_size, ssize alignment);
void  DefaultFree(void *ctx, void *ptr);
bool  DefaultTryExtend(void *ctx, void *ptr, ssize old_size, ssize new_size);

#endif //ALLOCATOR_H
