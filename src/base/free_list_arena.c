#include <string.h>

#include "free_list_arena.h"
#include "base/list.h"
#include "base/utils.h"

#define MIN_FREE_LIST_BLOCK_SIZE (ssize)sizeof(FreeBlock)

typedef struct {
    ssize free_block_address; // header may have been offset from free block base
    ssize allocation_size;
} AllocationHeader;

typedef struct FreeBlock {
    struct FreeBlock *next; // NOTE: these may only point to blocks in same buffer
    struct FreeBlock *prev;
    ssize total_size;
} FreeBlock;

typedef struct FreeListBuffer {
    FreeBlock *head;
    FreeBlock *tail;

    struct FreeListBuffer *next;
    struct FreeListBuffer *prev;

    ssize usable_size;
} FreeListBuffer;

typedef struct {
    FreeListBuffer *buffer;
    FreeBlock      *block;
    void           *user_ptr;
} BlockSearchResult;

static FreeBlock *write_free_block_header(void *address, ssize total_block_size)
{
    ASSERT(is_aligned((ssize)address, ALIGNOF(FreeBlock)));

    FreeBlock *result = address;
    result->total_size = total_block_size;

    return result;
}

FreeListArena fl_create(Allocator parent, ssize capacity)
{
    // Capacity must be rounded up since allocation sizes are rounded up to alignment boundary
    // of FreeBlock.
    ssize aligned_buffer_capacity = align_s64(capacity, ALIGNOF(FreeBlock));
    ssize block_offset_from_buffer_header = align_s64(SIZEOF(FreeListBuffer), ALIGNOF(FreeBlock));
    ssize total_alloc_size = aligned_buffer_capacity + block_offset_from_buffer_header;

    FreeListBuffer *first_buffer = allocate_aligned(parent, total_alloc_size, ALIGNOF(FreeListBuffer));
    void *first_free_block_address = byte_offset(first_buffer, block_offset_from_buffer_header);
    FreeBlock *first_free_block = write_free_block_header(first_free_block_address, aligned_buffer_capacity);

    ASSERT(block_offset_from_buffer_header >= SIZEOF(FreeListBuffer));
    ASSERT((ssize)first_free_block_address >= ((ssize)(first_buffer + 1)));
    ASSERT(is_aligned((ssize)first_free_block, ALIGNOF(FreeBlock)));

    first_buffer->usable_size = aligned_buffer_capacity;
    first_buffer->head = first_free_block;
    first_buffer->tail = first_free_block;

    FreeListArena result = {
        .head = first_buffer,
        .tail = first_buffer,
        .parent = parent
    };

    return result;
}

void fl_destroy(FreeListArena *arena)
{
    for (FreeListBuffer *buf = list_head(arena); buf;) {
        FreeListBuffer *next = buf->next;
        deallocate(arena->parent, buf);

        buf = next;
    }
}

static BlockSearchResult find_suitable_block(FreeListArena *arena, ssize bytes_requested,
    ssize requested_alignment)
{
    BlockSearchResult result = {0};

    // Ensure that result of subtracting sizeof(AllocationHeader) from user pointer is
    // properly aligned. Needed since a header will be written right before user pointer.
    ssize alignment = MAX(requested_alignment, ALIGNOF(AllocationHeader));

    for (FreeListBuffer *buffer = list_head(arena); buffer; buffer = list_next(buffer)) {
        for (FreeBlock *block = list_head(buffer); block; block = list_next(block)) {
            ssize first_usable_address = (ssize)byte_offset(block, sizeof(AllocationHeader));
            ssize aligned_alloc_address = align_s64(first_usable_address, alignment);

            ASSERT(is_aligned(aligned_alloc_address, alignment));

            ssize offset_from_base = aligned_alloc_address + bytes_requested - (ssize)block;

            if (offset_from_base <= block->total_size) {
                result.buffer = buffer;
                result.block = block;
                result.user_ptr = (void *)aligned_alloc_address;

                return result;
            }
        }
    }

    return result;
}

static void split_free_block(FreeListBuffer *buffer, FreeBlock *block, ssize split_offset)
{
    ASSERT(is_aligned((ssize)block, ALIGNOF(FreeBlock)));
    ASSERT(is_aligned(split_offset, ALIGNOF(FreeBlock)));

    FreeBlock *new_block = byte_offset(block, split_offset);
    ssize new_block_size = block->total_size - split_offset;

    ASSERT(new_block_size >= MIN_FREE_LIST_BLOCK_SIZE);
    ASSERT(split_offset < block->total_size);
    ASSERT(is_aligned((ssize)new_block, ALIGNOF(FreeBlock)));
    ASSERT(is_aligned(new_block_size, ALIGNOF(FreeBlock)));

    new_block->total_size = new_block_size;
    block->total_size = split_offset;

    list_insert_after(buffer, new_block, block);
}

static void remove_free_block(FreeListBuffer *buffer, FreeBlock *block)
{
    ASSERT(is_aligned((ssize)block, ALIGNOF(FreeBlock)));

    list_remove(buffer, block);
}

static void write_allocation_header(void *at_address, FreeBlock *free_block_address, ssize alloc_size)
{
    ASSERT(is_aligned((ssize)at_address, ALIGNOF(AllocationHeader)));
    ASSERT(is_aligned((ssize)free_block_address, ALIGNOF(FreeBlock)));
    ASSERT(is_aligned(alloc_size, ALIGNOF(FreeBlock)));

    AllocationHeader *header = at_address;
    header->free_block_address = (ssize)free_block_address;
    header->allocation_size = alloc_size;
}

void *fl_allocate(void *context, ssize item_count, ssize item_size, ssize alignment)
{
    ASSERT(is_pow2(alignment));

    FreeListArena *arena = context;
    void *result = 0;

    if (multiply_overflows_ssize(item_count, item_size)) {
        ASSERT(false);
        return 0;
    }

    // Allocation size must be rounded up to ensure that next free block lands on alignment boundary
    ssize allocation_size = align_s64(item_count * item_size, ALIGNOF(FreeBlock));

    BlockSearchResult search_result = find_suitable_block(arena, allocation_size, alignment);

    if (search_result.block) {
        result = search_result.user_ptr;

        ssize bytes_in_block_used = ptr_diff(result, search_result.block) + allocation_size;
        ssize block_remainder = search_result.block->total_size - bytes_in_block_used;

	ASSERT(is_aligned(block_remainder, ALIGNOF(FreeBlock)));
	ASSERT(is_aligned(bytes_in_block_used, ALIGNOF(FreeBlock)));
        ASSERT(byte_offset(result, allocation_size) != search_result.block->next);
        ASSERT(bytes_in_block_used <= search_result.block->total_size);

        if (block_remainder > MIN_FREE_LIST_BLOCK_SIZE) {
            split_free_block(search_result.buffer, search_result.block, bytes_in_block_used);
        } else {
            allocation_size += block_remainder; // Absorb remainder of block
        }

	ASSERT(is_aligned(allocation_size, ALIGNOF(FreeBlock)));
	ASSERT(allocation_size <= search_result.block->total_size);

        remove_free_block(search_result.buffer, search_result.block);

        void *alloc_header_address = byte_offset(result, -SIZEOF(AllocationHeader));
        write_allocation_header(alloc_header_address, search_result.block, allocation_size);
    } else {
        // TODO: dynamic growth
        ASSERT(false && "OOM");
    }

    mem_zero(result, allocation_size);

    return result;
}

typedef struct {
    FreeListBuffer *buffer;
    FreeBlock      *block;
} BlockPredecessor;

static FreeListBuffer *find_buffer_containing_address(FreeListArena *arena, void *address)
{
    ssize int_addr = (ssize)address;

    for (FreeListBuffer *buffer = list_head(arena); buffer; buffer = list_next(buffer)) {
        ssize buffer_address = (ssize)(buffer + 1);

        if ((buffer_address <= int_addr) && (int_addr < (buffer_address + buffer->usable_size))) {
            return buffer;
        }
    }

    return 0;
}

static BlockPredecessor find_free_block_preceding_address(FreeListArena *arena, void *address)
{
    ssize int_addr = (ssize)address;

    for (FreeListBuffer *buffer = list_head(arena); buffer; buffer = list_next(buffer)) {
        for (FreeBlock *block = list_head(buffer); block; block = list_next(block)) {
            ssize curr_address = (ssize)block;
            ssize next_address = (ssize)block->next;
            b32 is_preceding = (curr_address < int_addr) && ((int_addr < next_address) || (next_address == 0));

            if (is_preceding) {
                BlockPredecessor result = {
                    .buffer = buffer,
                    .block = block
                };

                return result;
            }
        }
    }

    return (BlockPredecessor){0};
}

static void merge_free_blocks(FreeListBuffer *buffer, FreeBlock *left, FreeBlock *right)
{
    ASSERT((ssize)left + left->total_size == (ssize)right);

    left->total_size += right->total_size;
    remove_free_block(buffer, right);
}

static void try_coalesce_blocks(FreeListBuffer *buffer, FreeBlock *middle)
{
    ASSERT(buffer);
    ASSERT(middle);

    FreeBlock *left = middle->prev;
    FreeBlock *right = middle->next;

    if (left && (((ssize)left + left->total_size) == (ssize)middle)) {
        merge_free_blocks(buffer, left, middle);

        middle = left;
    }

    if (right && (((ssize)middle + middle->total_size) == (ssize)right)) {
        merge_free_blocks(buffer, middle, right);
    }
}

void fl_deallocate(void *context, void *ptr)
{
    FreeListArena *arena = context;

    AllocationHeader *alloc_header = (AllocationHeader *)ptr - 1;
    void *free_block_address = (void *)alloc_header->free_block_address;
    ssize offset_from_free_block = ptr_diff(ptr, free_block_address);
    ssize free_block_size = alloc_header->allocation_size + offset_from_free_block;

    ASSERT(free_block_size != 0);

    FreeBlock *new_block = write_free_block_header(free_block_address, free_block_size);

    BlockPredecessor predecessor = find_free_block_preceding_address(arena, free_block_address);
    FreeListBuffer *containing_buffer = 0;

    if (predecessor.block) {
        ASSERT(predecessor.buffer);

        containing_buffer = predecessor.buffer;

        list_insert_after(containing_buffer, new_block, predecessor.block);
    } else {
        ASSERT(!predecessor.buffer);

        containing_buffer = find_buffer_containing_address(arena, free_block_address);
        ASSERT(containing_buffer);
        list_push_front(containing_buffer, new_block);
    }

    try_coalesce_blocks(containing_buffer, new_block);
}

ssize fl_get_memory_usage(FreeListArena *arena)
{
    ssize total_capacity = 0;

    for (FreeListBuffer *buffer = list_head(arena); buffer; buffer = list_next(buffer)) {
        total_capacity += buffer->usable_size;
    }

    ssize available_memory = fl_get_available_memory(arena);
    ssize result = total_capacity - available_memory;

    return result;
}

ssize fl_get_available_memory(FreeListArena *arena)
{
    ssize sum = 0;

    for (FreeListBuffer *buffer = list_head(arena); buffer; buffer = list_next(buffer)) {
        for (FreeBlock *block = list_head(buffer); block; block = list_next(block)) {
            // TODO: should alloc header size be subtracted? since this size can never actually be allocated
            sum += block->total_size;
        }
    }

    return sum;
}
