#include <stdlib.h>
#include <string.h>

#include "linear_arena.h"
#include "utils.h"

LinearArena LinearArena_Create(s64 capacity)
{
    byte *ptr = malloc(Cast_s64_usize(capacity));
    Assert(ptr);

    LinearArena result = {
        .memory = ptr,
        .used_size = 0,
        .capacity = capacity
    };

    return result;
}

void LinearArena_Destroy(LinearArena *arena)
{
    free(arena->memory);

    arena->memory = NULL;
    arena->used_size = 0;
    arena->capacity = 0;
}

static void *LinearArena_AllocBytes(void *context, s64 byte_count, s64 alignment)
{
    LinearArena *arena = context;
    const s64 aligned_top_offset = Align(arena->used_size, alignment);

    if ((aligned_top_offset + byte_count) > arena->capacity) {
        Assert(false);
    }

    byte *result = arena->memory + aligned_top_offset;
    arena->used_size += byte_count;

    Assert(IsAligned((s64)result, alignment));

    memset(result, 0, Cast_s64_usize(byte_count));

    return result;
}

void *LinearArena_Alloc(void *context, s64 count, s64 item_size, s64 alignment)
{
    LinearArena *arena = context;

    if (MultiplicationOverflows_s64(count, item_size)) {
        Assert(false);
        return NULL;
    }

    const s64 byte_count = count * item_size;

    return LinearArena_AllocBytes(arena, byte_count, alignment);
}

void LinearArena_Free(void *context, void *ptr)
{
    // no-op
    (void)context;
    (void)ptr;
}

void LinearArena_Reset(LinearArena* arena)
{
    arena->used_size = 0;
}

Allocator LinearArena_CreateAllocator(LinearArena* arena)
{
    Allocator allocator = {
        .alloc = LinearArena_Alloc,
        .dealloc = LinearArena_Free,
        .context = arena
    };

    return allocator;
}
