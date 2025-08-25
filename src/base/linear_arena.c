#include <string.h>

#include "linear_arena.h"
#include "utils.h"

static ArenaBlock *AllocateBlock(Allocator parent, s64 size)
{
    if (AdditionOverflows_ssize(size, sizeof(ArenaBlock))) {
        return 0;
    }

    s64 size_required = size + (s64)sizeof(ArenaBlock);

    ArenaBlock *block = AllocAligned(parent, size_required, AlignOf(ArenaBlock));
    block->next_block = 0;
    block->used_size = 0;
    block->capacity = size;

    // TODO: memzero macro/func
    MemZero(block + 1, Cast_s64_usize(size));

    return block;
}

LinearArena LinearArena_Create(Allocator parent, s64 capacity)
{
    ArenaBlock *block = AllocateBlock(parent, capacity);
    Assert(block);

    LinearArena result = {
        .parent = parent,
        .first_block = block,
        .top_block = block
    };

    return result;
}

void LinearArena_Destroy(LinearArena *arena)
{
    ArenaBlock *current_block = arena->first_block;

    while (current_block) {
        ArenaBlock *next_block = current_block->next_block;

        Free(arena->parent, current_block);

        current_block = next_block;
    }

    arena->first_block = 0;
    arena->top_block = 0;
}

static void *TryAllocateInBlock(ArenaBlock *block, s64 byte_count, s64 alignment)
{
    ssize block_address = (ssize)(block + 1);
    ssize top_address = block_address + block->used_size;
    ssize aligned_address = Align(top_address, alignment);
    ssize aligned_top_offset = aligned_address - block_address;

    if (AdditionOverflows_ssize(aligned_top_offset, byte_count)
    || ((aligned_top_offset + byte_count) > block->capacity)) {
        return 0;
    }

    byte *result = (byte *)(block_address + aligned_top_offset);
    block->used_size += byte_count;

    Assert(IsAligned((s64)result, alignment));

    MemZero(result, Cast_s64_usize(byte_count));

    return result;
}

void *AllocBytes(LinearArena *arena, s64 byte_count, s64 alignment)
{
    ArenaBlock *curr_block = arena->top_block;

    while (curr_block) {
        void *ptr = TryAllocateInBlock(curr_block, byte_count, alignment);

        if (ptr) {
            return ptr;
        }

        curr_block = curr_block->next_block;
    }

    // No space found, allocate a new block
    s64 aligned_header_size = Align(sizeof(ArenaBlock), alignment);
    s64 padding_required = (aligned_header_size - (s64)sizeof(ArenaBlock));
    s64 min_size_required = padding_required + byte_count;
    s64 new_block_size = Max(min_size_required, arena->top_block->capacity); // TODO: growth strategy?

    ArenaBlock *new_block = AllocateBlock(arena->parent, new_block_size);
    arena->top_block->next_block = new_block;
    arena->top_block = new_block;

    void *result = TryAllocateInBlock(new_block, byte_count, alignment);
    Assert(result);

    return result;
}

void *LinearArena_Alloc(void *context, s64 count, s64 item_size, s64 alignment)
{
    Assert(IsPow2(alignment));

    LinearArena *arena = context;

    // TODO: just crash when this happens
    if (MultiplicationOverflows_ssize(count, item_size)) {
        Assert(false);
        return 0;
    }

    const s64 byte_count = count * item_size;

    return AllocBytes(arena, byte_count, alignment);
}

void LinearArena_Free(void *context, void *ptr)
{
    // no-op
    (void)context;
    (void)ptr;
}

void LinearArena_Reset(LinearArena* arena)
{
    ArenaBlock *curr_block = arena->first_block;

    while (curr_block) {
        curr_block->used_size = 0;

        curr_block = curr_block->next_block;
    }

    arena->top_block = arena->first_block;
}

Allocator LinearArena_Allocator(LinearArena* arena)
{
    Allocator allocator = {
        .alloc = LinearArena_Alloc,
        .dealloc = LinearArena_Free,
        .context = arena
    };

    return allocator;
}
