#include "scratch.h"

#include "linear_arena.h"
#include "utils.h"

static THREAD_LOCAL LinearArena thread_scratch_arena;

TempArena temp_arena_begin()
{
    TempArena result = {0};

    b32 is_initialized = thread_scratch_arena.first_block != 0;

    if (!is_initialized) {
        thread_scratch_arena = la_create(default_allocator, MB(2));
    }

    result.arena = &thread_scratch_arena;
    result.start_top_block = thread_scratch_arena.top_block;
    result.start_offset_into_top_block = thread_scratch_arena.offset_into_top_block;

    return result;
}

void temp_arena_end(TempArena scratch)
{
    ASSERT(scratch.arena);
    ASSERT(scratch.start_top_block);
    ASSERT(scratch.start_offset_into_top_block >= 0);

    thread_scratch_arena.top_block = scratch.start_top_block;
    thread_scratch_arena.offset_into_top_block = scratch.start_offset_into_top_block;
}
