#ifndef ENTITY_ARENA_H
#define ENTITY_ARENA_H

#include "base/utils.h"

/*
  Entity arenas should be used for all allocations tied to an entity and are reset
  on entity being removed. This arena consists of a static array and pointers into it
  are just indices, meaning an entity can be serialized or copied and any pointers into the arena
  remain valid.

  Use caution when using this arena however as allocations are completely untyped.
*/

#define ENTITY_ARENA_SIZE MB(1)

#define entity_arena_allocate_item(arena, type)                 \
    entity_arena_allocate(arena, SIZEOF(type), ALIGNOF(type))

typedef struct {
    // This alignment should be large enough for any actual use case
    ALIGNAS(64) byte data[ENTITY_ARENA_SIZE];
    ssize offset;
} EntityArena;

typedef struct {
    ssize offset;
} EntityArenaIndex;

typedef struct {
    EntityArenaIndex index;
    void *ptr;
} EntityArenaAllocation;

EntityArenaAllocation entity_arena_allocate(EntityArena *arena, ssize byte_count, ssize alignment);
void *entity_arena_get(EntityArena *arena, EntityArenaIndex index);
void entity_arena_reset(EntityArena *arena);
ssize entity_arena_get_free_memory(EntityArena *arena);
ssize entity_arena_get_memory_usage(EntityArena *arena);

#endif //ENTITY_ARENA_H
