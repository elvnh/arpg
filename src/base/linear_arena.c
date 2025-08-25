#include <string.h>

#include "allocator.h"
#include "linear_arena.h"
#include "utils.h"

typedef struct ArenaBlock {
    struct ArenaBlock *next_block;
    ssize  capacity;
} ArenaBlock;

static ArenaBlock *AllocateBlock(Allocator parent, ssize size)
{
    if (AdditionOverflows_ssize(size, sizeof(ArenaBlock))) {
        return 0;
    }

    ssize size_required = size + (ssize)sizeof(ArenaBlock);

    ArenaBlock *block = AllocAligned(parent, size_required, AlignOf(ArenaBlock));
    block->next_block = 0;
    block->capacity = size;

    MemZero(block + 1, Cast_s64_usize(size));

    return block;
}

static void SwitchToBlock(LinearArena *arena, ArenaBlock *block)
{
    arena->offset_into_top_block = 0;
    arena->top_block = block;
}

LinearArena LinearArena_Create(Allocator parent, ssize capacity)
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
    arena->offset_into_top_block = 0;
}

static void *TryAllocateInTopBlock(LinearArena *arena, ssize byte_count, ssize alignment)
{
    ssize block_address = (ssize)(arena->top_block + 1);
    ssize top_address = block_address + arena->offset_into_top_block;
    ssize aligned_address = Align(top_address, alignment);
    ssize aligned_top_offset = aligned_address - block_address;

    if (AdditionOverflows_ssize(aligned_top_offset, byte_count)
    || ((aligned_top_offset + byte_count) > arena->top_block->capacity)) {
        return 0;
    }

    byte *result = (byte *)(block_address + aligned_top_offset);
    Assert(IsAligned((ssize)result, alignment));

    MemZero(result, Cast_s64_usize(byte_count));

    arena->offset_into_top_block = aligned_top_offset + byte_count;

    return result;
}

void *AllocBytes(LinearArena *arena, ssize byte_count, ssize alignment)
{
    void *result = TryAllocateInTopBlock(arena, byte_count, alignment);

    while (!result && arena->top_block->next_block) {
        SwitchToBlock(arena, arena->top_block->next_block);

        result = TryAllocateInTopBlock(arena, byte_count, alignment);
    }

    if (!result) {
        // No space found, allocate a new block
        ssize aligned_header_size = Align(sizeof(ArenaBlock), alignment);
        ssize padding_required = (aligned_header_size - (ssize)sizeof(ArenaBlock));
        ssize min_size_required = padding_required + byte_count;
        ssize new_block_size = Max(min_size_required, arena->top_block->capacity); // TODO: growth strategy?

        ArenaBlock *new_block = AllocateBlock(arena->parent, new_block_size);
        arena->top_block->next_block = new_block;
        SwitchToBlock(arena, new_block);

        result = TryAllocateInTopBlock(arena, byte_count, alignment);
        Assert(result);
    }

    Assert(result);

    return result;
}

void *LinearArena_Alloc(void *context, ssize count, ssize item_size, ssize alignment)
{
    Assert(IsPow2(alignment));

    LinearArena *arena = context;

    // TODO: just crash when this happens
    if (MultiplicationOverflows_ssize(count, item_size)) {
        Assert(false);
        return 0;
    }

    ssize byte_count = count * item_size;

    return AllocBytes(arena, byte_count, alignment);
}

void LinearArena_Reset(LinearArena* arena)
{
    SwitchToBlock(arena, arena->first_block);
}

bool LinearArena_TryExtend(void *context, void *ptr, ssize old_size, ssize new_size)
{
    LinearArena *arena = context;

    Assert(new_size > old_size);

    // NOTE: extending only works if this is the last allocation, hence only checking top block
    void *current_top = ByteOffset(arena->top_block + 1, arena->offset_into_top_block);

    if (ByteOffset(ptr, old_size) == current_top) {
        void *result = TryAllocateInTopBlock(arena, new_size - old_size, 1);

        if (result) {
            return true;
        }
    }

    return false;
}

Allocator LinearArena_Allocator(LinearArena* arena)
{
    Allocator allocator = {
        .alloc = LinearArena_Alloc,
        .dealloc = DefaultFree,
        .try_extend = LinearArena_TryExtend,
        .context = arena
    };

    return allocator;
}
