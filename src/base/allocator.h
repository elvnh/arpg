#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdlib.h>

#include "utils.h"

#define AllocArray(alloc, type, count) (type *)(Allocate(alloc, count, sizeof(type), AlignOf(type)))
#define AllocItem(alloc, type) AllocArray(alloc, type, 1)
#define Alloc(alloc, bytes) (void *)AllocArray(alloc, byte, bytes)
#define AllocAligned(alloc, size, alignment) (void *)Allocate(alloc, size, 1, alignment)

// TODO: make into const global variable
#define DefaultAllocator (Allocator){ .alloc = DefaultAllocate, .dealloc = DefaultFree }

typedef void *(*AllocFunction)(void*, s64, s64, s64);
typedef void  (*FreeFunction)(void*, void*);

typedef struct {
    AllocFunction alloc;
    FreeFunction  dealloc;

    void *context;
} Allocator;

// TODO: move into implementation file
static inline void *Allocate(Allocator allocator, s64 item_count, s64 item_size, s64 alignment)
{
    return allocator.alloc(allocator.context, item_count, item_size, alignment);
}

static inline void Free(Allocator allocator, void *ptr)
{
    allocator.dealloc(allocator.context, ptr);
}

static inline void *DefaultAllocate(void *ctx, s64 item_count, s64 item_size, s64 alignment)
{
    (void)ctx;
    (void)alignment;

    if (MultiplicationOverflows_ssize(item_count, item_size)) {
        return 0;
    }

    const s64 byte_count = item_count * item_size;

    void *ptr = malloc(Cast_s64_usize(byte_count));
    Assert(IsAligned((s64)ptr, alignment));

    return ptr;
}

static inline void DefaultFree(void *ctx, void *ptr)
{
    (void)ctx;
    free(ptr);
}

#endif //ALLOCATOR_H
