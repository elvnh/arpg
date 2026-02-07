#include "entity_arena.h"
#include "base/utils.h"

#include <string.h>

EntityArenaAllocation entity_arena_allocate(EntityArena *arena, ssize byte_count, ssize alignment)
{
    ASSERT(byte_count > 0);
    ASSERT(is_pow2(alignment));
    ASSERT(alignment <= ALIGNOF(arena->data));

    ssize aligned_index = align(arena->offset, alignment);
    ASSERT(aligned_index < ENTITY_ARENA_SIZE);
    ASSERT(!add_overflows_ssize(aligned_index, byte_count));

    ssize new_offset = aligned_index + byte_count;
    ASSERT(new_offset <= ENTITY_ARENA_SIZE);

    arena->offset = new_offset;

    EntityArenaAllocation result = {0};
    result.index = (EntityArenaIndex){aligned_index};
    result.ptr = entity_arena_get(arena, result.index);

    memset(result.ptr, 0, (usize)byte_count);

    return result;
}

void *entity_arena_get(EntityArena *arena, EntityArenaIndex index)
{
    ASSERT(index.offset >= 0);
    ASSERT(index.offset < ENTITY_ARENA_SIZE);

    void *result = arena->data + index.offset;

    return result;
}

void entity_arena_reset(EntityArena *arena)
{
    arena->offset = 0;
}

ssize entity_arena_get_free_memory(EntityArena *arena)
{
    ssize result = ENTITY_ARENA_SIZE - arena->offset;
    return result;
}

ssize entity_arena_get_memory_usage(EntityArena *arena)
{
    ssize result = arena->offset;
    return result;
}
