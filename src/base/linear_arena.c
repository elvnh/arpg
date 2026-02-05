#include <string.h>

#include "allocator.h"
#include "linear_arena.h"
#include "utils.h"

typedef struct ArenaBlock {
    struct ArenaBlock *next_block;
    struct ArenaBlock *prev_block;
    ssize  capacity;
} ArenaBlock;

static ArenaBlock *allocate_block(Allocator parent, ArenaBlock *previous, ssize size)
{
    if (add_overflows_ssize(size, sizeof(ArenaBlock))) {
        return 0;
    }

    ssize size_required = size + (ssize)sizeof(ArenaBlock);

    ArenaBlock *block = allocate_aligned(parent, size_required, 1, ALIGNOF(ArenaBlock));
    block->next_block = 0;
    block->prev_block = previous;
    block->capacity = size;

    mem_zero(block + 1, ssize_to_usize(size));

    return block;
}

static void switch_to_block(LinearArena *arena, ArenaBlock *block)
{
    arena->offset_into_top_block = 0;
    arena->top_block = block;
}

LinearArena la_create(Allocator parent, ssize capacity)
{
    ArenaBlock *block = allocate_block(parent, 0, capacity);
    ASSERT(block);

    LinearArena result = {
        .parent = parent,
        .first_block = block,
        .top_block = block
    };

    return result;
}

void la_destroy(LinearArena *arena)
{
    ArenaBlock *current_block = arena->first_block;

    while (current_block) {
        ArenaBlock *next_block = current_block->next_block;

        deallocate(arena->parent, current_block);

        current_block = next_block;
    }

    arena->first_block = 0;
    arena->top_block = 0;
    arena->offset_into_top_block = 0;
}

static void *try_allocate_in_top_block(LinearArena *arena, ssize byte_count, ssize alignment)
{
    ssize block_address = (ssize)(arena->top_block + 1);
    ssize top_address = block_address + arena->offset_into_top_block;
    ssize aligned_address = align(top_address, alignment);
    ssize aligned_top_offset = aligned_address - block_address;

    if (add_overflows_ssize(aligned_top_offset, byte_count)
    || ((aligned_top_offset + byte_count) > arena->top_block->capacity)) {
        return 0;
    }

    byte *result = (byte *)(block_address + aligned_top_offset);
    ASSERT(is_aligned((ssize)result, alignment));

    mem_zero(result, ssize_to_usize(byte_count));

    arena->offset_into_top_block = aligned_top_offset + byte_count;

    return result;
}

void *alloc_bytes(LinearArena *arena, ssize byte_count, ssize alignment)
{
    ASSERT(byte_count > 0);

    void *result = try_allocate_in_top_block(arena, byte_count, alignment);

    while (!result && arena->top_block->next_block) {
        switch_to_block(arena, arena->top_block->next_block);

        result = try_allocate_in_top_block(arena, byte_count, alignment);
    }

    if (!result) {
        // No space found, allocate a new block
        ssize aligned_header_size = align(sizeof(ArenaBlock), alignment);
        ssize padding_required = (aligned_header_size - (ssize)sizeof(ArenaBlock));
        ssize min_size_required = padding_required + byte_count;
        ssize new_block_size = MAX(min_size_required, arena->top_block->capacity); // TODO: growth strategy?

        ArenaBlock *new_block = allocate_block(arena->parent, arena->top_block, new_block_size);
        arena->top_block->next_block = new_block;
        switch_to_block(arena, new_block);

        result = try_allocate_in_top_block(arena, byte_count, alignment);
        ASSERT(result);
    }

    ASSERT(result);

    return result;
}

void *la_allocate(void *context, ssize count, ssize item_size, ssize alignment)
{
    ASSERT(is_pow2(alignment));
    ASSERT(count > 0);
    ASSERT(item_size > 0);
    ASSERT(alignment > 0);

    LinearArena *arena = context;

    // TODO: just crash when this happens
    ASSERT(!multiply_overflows_ssize(count, item_size));

    ssize byte_count = count * item_size;

    return alloc_bytes(arena, byte_count, alignment);
}

void la_reset(LinearArena* arena)
{
    ASSERT(arena->first_block);
    ASSERT(arena->top_block);

    switch_to_block(arena, arena->first_block);
}

#if 0
static bool arena_try_resize_allocation(void *context, void *ptr, ssize old_size, ssize new_size)
{
    ASSERT(new_size >= 0);

    LinearArena *arena = context;
    // NOTE: extending only works if this is the last allocation, hence only checking top block
    void *current_top = byte_offset(arena->top_block + 1, arena->offset_into_top_block);

    if (byte_offset(ptr, old_size) == current_top) {
        // TODO: later if popping is implemented, do that here
        ASSERT(new_size);

        if (new_size > old_size) {
            void *result = try_allocate_in_top_block(arena, new_size - old_size, 1);

            if (result) {
                return true;
            }
        } else {
            arena->offset_into_top_block -= old_size - new_size;

            return true;
        }
    }

    return false;
}
#endif

Allocator la_allocator(LinearArena* arena)
{
    Allocator allocator = {
        .alloc = la_allocate,
        .dealloc = stub_deallocate,
        .context = arena
    };

    return allocator;
}

ssize la_get_memory_usage(LinearArena *arena)
{
    ssize sum = 0;
    ArenaBlock *block = arena->first_block;

    while (block != arena->top_block) {
        sum += block->capacity;

        block = block->next_block;
    }

    sum += arena->offset_into_top_block;

    return sum;
}

void la_pop_to(LinearArena *arena, void *ptr)
{
    ArenaBlock *curr_block = arena->top_block;

    ssize int_ptr = (ssize)ptr;

    while (curr_block) {
        ssize offset = int_ptr - (ssize)(curr_block + 1);

        if ((offset >= 0) && (offset < curr_block->capacity)) {
            arena->offset_into_top_block = offset;
            arena->top_block = curr_block;

            break;
        }

        curr_block = curr_block->prev_block;
    }

    ASSERT(curr_block);
}

void *la_copy_allocation(void *context, void *arr, ssize item_count, ssize item_size, ssize alignment)
{
    void *new_alloc = la_allocate(context, item_count, item_size, alignment);
    memcpy(new_alloc, arr, ssize_to_usize(item_count * item_size));

    return new_alloc;
}
