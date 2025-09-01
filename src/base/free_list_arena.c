#include <string.h>

#include "free_list_arena.h"
#include "base/list.h"
#include "base/utils.h"

#define MIN_FREE_LIST_ALLOC_SIZE (ssize)sizeof(FreeBlock)

typedef struct {
    ssize free_block_address; // header may have been offset from free block base
    ssize allocation_size;
} AllocationHeader;

typedef struct FreeBlock {
    struct FreeBlock *next;
    struct FreeBlock *prev;
    ssize total_size;
} FreeBlock;

typedef struct {
    FreeBlock *block;
    void      *user_ptr;
} BlockSearchResult;

FreeListArena fl_create(Allocator parent, ssize capacity)
{
    byte *memory = allocate_array(parent, byte, capacity);

    FreeBlock *first_block = (FreeBlock *)memory;
    first_block->total_size = capacity;

    FreeListArena result = {
        .memory = memory,
        .head = first_block,
        .tail = first_block,
        .capacity = capacity,
        .parent = parent
    };

    return result;
}

static BlockSearchResult find_suitable_block(FreeListArena *arena, ssize bytes_requested,
    ssize requested_alignment)
{
    BlockSearchResult result = {0};

    // Ensure that it's safe to subtract sizeof(AllocationHeader) from user pointer
    ssize alignment = MAX(requested_alignment, ALIGNOF(AllocationHeader));

    for (FreeBlock *block = list_head(arena); block; block = list_next(block)) {
        ssize first_usable_address = (ssize)byte_offset(block, sizeof(AllocationHeader));
        ssize aligned_alloc_address = align_s64(first_usable_address, alignment);

        ASSERT(is_aligned(aligned_alloc_address, alignment));

        ssize offset_from_base = aligned_alloc_address + bytes_requested - (ssize)block;

        if (offset_from_base <= block->total_size) {
            result.block = block;
            result.user_ptr = (void *)aligned_alloc_address;

            return result;
        }
    }

    return result;
}

static void split_free_block(FreeListArena *arena, FreeBlock *block, ssize bytes_in_block_used)
{
    ASSERT(bytes_in_block_used >= MIN_FREE_LIST_ALLOC_SIZE);
    ASSERT(block->total_size - bytes_in_block_used >= MIN_FREE_LIST_ALLOC_SIZE);

    FreeBlock *new_block = byte_offset(block, bytes_in_block_used);
    new_block->total_size = block->total_size - bytes_in_block_used;

    block->total_size = bytes_in_block_used;

    list_insert_after(arena, new_block, block);
}

static void remove_free_block(FreeListArena *arena, FreeBlock *block)
{
    list_remove(arena, block);
}

static void write_allocation_header(void *address, FreeBlock *free_block_address, ssize alloc_size)
{
    AllocationHeader *header = address;
    header->free_block_address = (ssize)free_block_address;
    header->allocation_size = alloc_size;
}

void *fl_allocate(void *context, ssize item_count, ssize item_size, ssize alignment)
{
    ASSERT(is_pow2(alignment));

    FreeListArena *arena = context;

    if (multiply_overflows_ssize(item_count, item_size)) {
        ASSERT(false);
        return 0;
    }

    ssize bytes_required = MAX(MIN_FREE_LIST_ALLOC_SIZE, item_count * item_size);

    BlockSearchResult search_result = find_suitable_block(arena, bytes_required, alignment);
    ASSERT(is_aligned((ssize)search_result.user_ptr, alignment));

    void *user_ptr = 0;

    if (search_result.block) {
        ssize bytes_in_block_used = ptr_diff(search_result.user_ptr, search_result.block) + bytes_required;
        ssize block_remainder = search_result.block->total_size - bytes_in_block_used;

        ASSERT(byte_offset(search_result.user_ptr, bytes_required) != search_result.block->next);
        ASSERT(bytes_in_block_used <= search_result.block->total_size);

        if (block_remainder > MIN_FREE_LIST_ALLOC_SIZE) {
            split_free_block(arena, search_result.block, bytes_in_block_used);
        }

        remove_free_block(arena, search_result.block);

        void *alloc_header_address = byte_offset(search_result.user_ptr, -(SIZEOF(AllocationHeader)));
        write_allocation_header(alloc_header_address, search_result.block, bytes_required);

        user_ptr = search_result.user_ptr;
    } else {
        ASSERT(false);
    }

    mem_zero(user_ptr, bytes_required);

    return user_ptr;
}

static FreeBlock *find_free_block_preceding_address(FreeListArena *arena, void *address)
{
    ssize int_addr = (ssize)address;

    for (FreeBlock *block = list_head(arena); block; block = list_next(block)) {
        ssize curr_address = (ssize)block;
        ssize next_address = (ssize)block->next;
        b32 is_preceding = (curr_address < int_addr) && ((int_addr < next_address) || (next_address == 0));

        if (is_preceding) {
            return block;
        }
    }

    return 0;
}

static FreeBlock *write_free_block_header(void *address, ssize total_block_size)
{
    FreeBlock *result = address;
    result->total_size = total_block_size;

    return result;
}

void fl_deallocate(void *context, void *ptr)
{
    FreeListArena *arena = context;

    AllocationHeader *alloc_header = (AllocationHeader *)ptr - 1;
    void *free_block_address = (void *)alloc_header->free_block_address;
    ssize offset_from_free_block = ptr_diff(ptr, free_block_address);
    ssize free_block_size = alloc_header->allocation_size + offset_from_free_block;

    ASSERT(free_block_size != 0);

    FreeBlock *predecessor = find_free_block_preceding_address(arena, free_block_address);
    FreeBlock *new_block = write_free_block_header(free_block_address, free_block_size);

    if (predecessor) {
        list_insert_after(arena, new_block, predecessor);
    } else {
        list_push_front(arena, new_block);
    }
}
