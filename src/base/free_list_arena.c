#include <string.h>

#include "free_list_arena.h"
#include "base/utils.h"
#include "list.h"

#define MIN_FREE_LIST_BLOCK_SIZE (SIZEOF(FreeBlock))

typedef struct {
    ssize free_block_address; // Header may have been offset from free block base
    ssize allocation_size;
} AllocationHeader;

typedef struct FreeBlock {
    struct FreeBlock *next; // NOTE: These may only point to blocks in same buffer
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
    ASSERT(address);
    ASSERT(total_block_size > 0);

    FreeBlock *result = address;
    result->total_size = total_block_size;

    return result;
}

static FreeListBuffer *allocate_new_buffer(FreeListArena *arena, ssize capacity)
{
    // Capacity must be rounded up since allocation sizes are rounded up to alignment boundary
    // of FreeBlock.
    ssize aligned_buffer_capacity = align(capacity, ALIGNOF(FreeBlock));
    ssize block_offset_from_buffer_header = align(SIZEOF(FreeListBuffer), ALIGNOF(FreeBlock));
    ssize total_alloc_size = aligned_buffer_capacity + block_offset_from_buffer_header;

    FreeListBuffer *new_buffer = allocate_aligned(arena->parent, total_alloc_size, 1, ALIGNOF(FreeListBuffer));

    void *first_free_block_address = byte_offset(new_buffer, block_offset_from_buffer_header);
    FreeBlock *first_free_block = write_free_block_header(first_free_block_address, aligned_buffer_capacity);

    ASSERT(block_offset_from_buffer_header >= SIZEOF(FreeListBuffer));
    ASSERT((ssize)first_free_block_address >= ((ssize)(new_buffer + 1)));
    ASSERT(is_aligned((ssize)first_free_block, ALIGNOF(FreeBlock)));

    new_buffer->usable_size = aligned_buffer_capacity;
    new_buffer->head = first_free_block;
    new_buffer->tail = first_free_block;

    return new_buffer;
}

FreeListArena fl_create(Allocator parent, ssize capacity)
{
    FreeListArena result = {
        .head = 0,
        .tail = 0,
        .parent = parent
    };

    FreeListBuffer *first_buffer = allocate_new_buffer(&result, capacity);
    list_push_back(&result, first_buffer);

    return result;
}

void fl_destroy(FreeListArena *arena)
{
    for (FreeListBuffer *buf = list_head(arena); buf;) {
        FreeListBuffer *next = buf->next;
        deallocate(arena->parent, buf);

        buf = next;
    }

    *arena = (FreeListArena){0};
}

static void *try_get_aligned_alloc_address_in_block(FreeBlock *block, ssize size, ssize alignment)
{
    ssize first_usable_address = (ssize)byte_offset(block, sizeof(AllocationHeader));
    ssize aligned_alloc_address = align(first_usable_address, alignment);
    ssize alloc_end_address = aligned_alloc_address + size - (ssize)block;

    if (alloc_end_address <= block->total_size) {
        return (void *)aligned_alloc_address;
    }

    return 0;
}

static ssize calculate_next_buffer_size(FreeListArena *arena, ssize allocation_size)
{
    ssize result = MAX(list_tail(arena)->usable_size, allocation_size * 2 + SIZEOF(AllocationHeader));

    return result;
}

static BlockSearchResult find_suitable_block(FreeListArena *arena, ssize bytes_requested,
    ssize alignment)
{
    BlockSearchResult result = {0};

    for (FreeListBuffer *buffer = list_head(arena); buffer; buffer = list_next(buffer)) {
        for (FreeBlock *block = list_head(buffer); block; block = list_next(block)) {
            void *alloc_address = try_get_aligned_alloc_address_in_block(block, bytes_requested, alignment);

            if (alloc_address) {
                result.buffer = buffer;
                result.block = block;
                result.user_ptr = alloc_address;

                return result;
            }
        }
    }

    return result;
}

static void split_free_block(FreeListBuffer *buffer, FreeBlock *block, ssize split_offset)
{
    ASSERT(split_offset > 0);
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

static void write_allocation_header(ssize at_address, FreeBlock *free_block_address, ssize alloc_size)
{
    ASSERT(is_aligned((ssize)at_address, ALIGNOF(AllocationHeader)));
    ASSERT(is_aligned((ssize)free_block_address, ALIGNOF(FreeBlock)));
    ASSERT(is_aligned(alloc_size, ALIGNOF(FreeBlock)));

    AllocationHeader *header = (AllocationHeader *)at_address;
    header->free_block_address = (ssize)free_block_address;
    header->allocation_size = alloc_size;
}

static void allocate_from_free_block(void *alloc_address, ssize alloc_size,
    FreeBlock *block, FreeListBuffer *buffer)
{
    ASSERT(alloc_address);
    ASSERT(block);
    ASSERT(buffer);
    ASSERT(alloc_size);
    ASSERT(alloc_size <= block->total_size);
    ASSERT(((ssize)alloc_address + alloc_size) <= ((ssize)block + block->total_size));

    ssize bytes_in_block_used = ptr_diff(alloc_address, block) + alloc_size;
    ssize block_remainder = block->total_size - bytes_in_block_used;

    ASSERT(is_aligned(block_remainder, ALIGNOF(FreeBlock)));
    ASSERT(is_aligned(bytes_in_block_used, ALIGNOF(FreeBlock)));
    ASSERT(byte_offset(alloc_address, alloc_size) != block->next);
    ASSERT(bytes_in_block_used <= block->total_size);

    if (block_remainder > MIN_FREE_LIST_BLOCK_SIZE) {
        split_free_block(buffer, block, bytes_in_block_used);
    } else {
        alloc_size += block_remainder; // Absorb remainder of block
    }

    ASSERT(is_aligned(alloc_size, ALIGNOF(FreeBlock)));
    ASSERT(alloc_size <= block->total_size);

    // TODO: switch order of params
    remove_free_block(buffer, block);

    ssize alloc_header_address = (ssize)alloc_address - SIZEOF(AllocationHeader);
    write_allocation_header(alloc_header_address, block, alloc_size);
}

void *fl_allocate(void *context, ssize item_count, ssize item_size, ssize requested_alignment)
{
    ASSERT(is_pow2(requested_alignment));

    FreeListArena *arena = context;

    if (multiply_overflows_ssize(item_count, item_size)) {
        ASSERT(false);
        return 0;
    }

    // Allocation size must be rounded up to ensure that next free block lands on alignment boundary
    ssize allocation_size = align(item_count * item_size, ALIGNOF(FreeBlock));

    // Ensure that result of subtracting sizeof(AllocationHeader) from user pointer is
    // properly aligned. Needed since a header will be written right before user pointer.
    ssize alignment = MAX(requested_alignment, ALIGNOF(AllocationHeader));

    void *alloc_address = 0; // TODO: rename
    FreeBlock *block_of_allocation = 0;
    FreeListBuffer *buffer_of_allocation = 0;

    BlockSearchResult search_result = find_suitable_block(arena, allocation_size, alignment);

    if (search_result.block) {
        alloc_address = search_result.user_ptr;
        block_of_allocation = search_result.block;
        buffer_of_allocation = search_result.buffer;
    } else {
        ssize next_buffer_size = calculate_next_buffer_size(arena, allocation_size);
        FreeListBuffer *new_buffer = allocate_new_buffer(arena, next_buffer_size);

        list_push_back(arena, new_buffer);

        alloc_address = try_get_aligned_alloc_address_in_block(list_head(new_buffer),
	    allocation_size, alignment);
        block_of_allocation = list_head(new_buffer);
        buffer_of_allocation = new_buffer;
    }

    allocate_from_free_block(alloc_address, allocation_size, block_of_allocation, buffer_of_allocation);
    mem_zero(alloc_address, allocation_size);

    return alloc_address;
}

static FreeListBuffer *find_buffer_containing_address(FreeListArena *arena, ssize address)
{
    for (FreeListBuffer *buffer = list_head(arena); buffer; buffer = list_next(buffer)) {
        ssize buffer_address = (ssize)(buffer + 1);

        if ((buffer_address <= address) && (address < (buffer_address + buffer->usable_size))) {
            return buffer;
        }
    }

    ASSERT(false);
    return 0;
}

static FreeBlock *find_free_block_preceding_address(ssize address, FreeListBuffer *buffer)
{
    for (FreeBlock *block = list_head(buffer); block; block = list_next(block)) {
        ssize curr_address = (ssize)block;
        ssize next_address = (ssize)block->next;

        b32 is_preceding = (curr_address < address) && ((address < next_address) || (next_address == 0));

        if (is_preceding) {
            return block;
        }
    }

    return 0;
}

static FreeBlock *find_free_block_succeeding_address(ssize address, FreeListBuffer *buffer)
{
    ASSERT(address > 0);
    ASSERT(buffer);

    for (FreeBlock *block = list_head(buffer); block; block = list_next(block)) {
        ssize curr_address = (ssize)block;
        ssize prev_address = (ssize)block->prev;

	b32 is_successor = (prev_address < address) && (address < curr_address);

	if (is_successor) {
	    return block;
	}
    }

    return 0;
}

static void merge_free_blocks(FreeListBuffer *buffer, FreeBlock *left, FreeBlock *right)
{
    ASSERT((ssize)left + left->total_size == (ssize)right);

    left->total_size += right->total_size;
    remove_free_block(buffer, right);
}

static void try_coalesce_free_blocks(FreeListBuffer *buffer, FreeBlock *middle)
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

static AllocationHeader *get_allocation_header(void *alloc_address)
{
    ASSERT(alloc_address);
    AllocationHeader *result = (AllocationHeader *)alloc_address - 1;

    return result;
}

void fl_deallocate(void *context, void *ptr)
{
    FreeListArena *arena = context;

    AllocationHeader *alloc_header = get_allocation_header(ptr);
    void *free_block_address = (void *)alloc_header->free_block_address;
    ssize offset_from_free_block = ptr_diff(ptr, free_block_address);
    ssize free_block_size = alloc_header->allocation_size + offset_from_free_block;

    ASSERT(free_block_size > 0);

    FreeBlock *new_block = write_free_block_header(free_block_address, free_block_size);

    FreeListBuffer *containing_buffer = find_buffer_containing_address(arena, (ssize)free_block_address);
    FreeBlock *predecessor = find_free_block_preceding_address((ssize)free_block_address, containing_buffer);

    ASSERT(containing_buffer);

    if (predecessor) {
        list_insert_after(containing_buffer, new_block, predecessor);
    } else {
        list_push_front(containing_buffer, new_block);
    }

    try_coalesce_free_blocks(containing_buffer, new_block);

#if 1
    // Check that all blocks are coalesced
    if (fl_get_memory_usage(arena) == 0) {
	for (FreeListBuffer *buf = list_head(arena); buf; buf = list_next(buf)) {
	    ASSERT(buf->head == buf->tail);
	}
    }
#endif
}

static b32 fl_try_resize_allocation(FreeListArena *arena, void *ptr, ssize new_size)
{
    ASSERT(new_size >= 0);

    AllocationHeader *alloc_header = get_allocation_header(ptr);
    FreeListBuffer *buffer = find_buffer_containing_address(arena, (ssize)ptr);
    FreeBlock *successor_block = find_free_block_succeeding_address((ssize)ptr, buffer);

    ssize old_alloc_size = alloc_header->allocation_size;
    ssize aligned_new_alloc_size = align(new_size, ALIGNOF(FreeBlock)); // TODO: create function for this

    ASSERT(is_aligned(old_alloc_size, ALIGNOF(FreeBlock)));
    ASSERT(old_alloc_size > 0);

    bool success = false;

    if (aligned_new_alloc_size == 0) {
	fl_deallocate(arena, ptr);

	success = true;
    } else if (aligned_new_alloc_size == old_alloc_size) {
	success = true;
    } else if (aligned_new_alloc_size < old_alloc_size) {
	ssize new_block_size = old_alloc_size - aligned_new_alloc_size;
	ASSERT(is_aligned(new_block_size, ALIGNOF(FreeBlock)));

	if (new_block_size >= MIN_FREE_LIST_BLOCK_SIZE) {
	    void *new_block_address = byte_offset(ptr, aligned_new_alloc_size);
	    ASSERT(is_aligned((ssize)new_block_address, ALIGNOF(FreeBlock)));

	    FreeBlock *new_block = write_free_block_header(new_block_address, new_block_size);
	    list_insert_before(buffer, new_block, successor_block);

	    try_coalesce_free_blocks(buffer, new_block);

	    alloc_header->allocation_size = aligned_new_alloc_size;
	}

	success = true;
    } else {
	ssize bytes_needed_from_next_block = aligned_new_alloc_size - old_alloc_size;
	ssize allocation_end = (ssize)ptr + old_alloc_size;
	b32 is_adjacent = allocation_end == (ssize)successor_block;

	if (is_adjacent && (bytes_needed_from_next_block <= successor_block->total_size)) {
	    split_free_block(buffer, successor_block, bytes_needed_from_next_block);
	    remove_free_block(buffer, successor_block);

	    alloc_header->allocation_size = aligned_new_alloc_size;

	    success = true;
	}
    }

#if 1 // Debug
	// Check that all blocks are coalesced
	if (fl_get_memory_usage(arena) == 0) {
	    for (FreeListBuffer *buf = list_head(arena); buf; buf = list_next(buf)) {
		ASSERT(buf->head == buf->tail);
	    }
	}
#endif

    return success;
}

void *fl_reallocate(void *context, void *ptr, ssize new_count, ssize item_size, ssize alignment)
{
    ASSERT(ptr);
    ASSERT(context);

    if (multiply_overflows_ssize(new_count, item_size)) {
	ASSERT(0);
	return 0;
    }

    FreeListArena *arena = context;
    AllocationHeader *alloc_header = get_allocation_header(ptr);
    ASSERT(alloc_header->allocation_size > 0);

    ssize old_size = alloc_header->allocation_size;
    ssize new_size = new_count * item_size;

    void *result = ptr;
    b32 resized_in_place = fl_try_resize_allocation(arena, ptr, new_size);

    if (!resized_in_place) {
	ASSERT(new_size > alloc_header->allocation_size);

	result = fl_allocate(arena, new_count, item_size, alignment);
	memcpy(result, ptr, (usize)old_size);
	fl_deallocate(arena, ptr);
    }

    return result;
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

Allocator fl_allocator(FreeListArena *arena)
{
    Allocator result = {
        .context = arena,
        .alloc = fl_allocate,
        .dealloc = fl_deallocate
    };

    return result;
}
