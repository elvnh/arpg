#include "scratch.h"

#include "linear_arena.h"
#include "utils.h"

static THREAD_LOCAL LinearArena thread_scratch_arena;

LinearArena temp_arena_begin()
{
    LinearArena result = {0};

    b32 is_initialized = thread_scratch_arena.first_block != 0;

    if (!is_initialized) {
        thread_scratch_arena = la_create(default_allocator, MB(8));
    }

    result = la_create(la_allocator(&thread_scratch_arena), KB(64));

    return result;
}

void temp_arena_end(LinearArena *arena)
{
    la_destroy(arena);
    *arena = zero_struct(LinearArena);
}
