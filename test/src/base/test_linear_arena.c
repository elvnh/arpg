#include "base/allocator.h"
#include "base/linear_arena.h"
#include "base/utils.h"
#include "test_macros.h"

// TODO: test parent arenas

TEST_CASE(linear_arena_create)
{
    LinearArena arena = la_create(default_allocator, 1024);

    REQUIRE(la_get_memory_usage(&arena) == 0);

    la_destroy(&arena);
}

TEST_CASE(linear_arena_alloc_basic)
{
    LinearArena arena = la_create(default_allocator, 1024);

    s32 *arr = la_allocate(&arena, 10, SIZEOF(s32), ALIGNOF(s32));
    REQUIRE(la_get_memory_usage(&arena) >= 10 * SIZEOF(*arr));
    REQUIRE(is_aligned((s64)arr, ALIGNOF(s32)));

    s32 *next = la_allocate(&arena, 10, SIZEOF(s32), ALIGNOF(s32));
    REQUIRE(arr != next);

    la_destroy(&arena);
}

TEST_CASE(linear_arena_alloc_aligned)
{
    LinearArena arena = la_create(default_allocator, 1024);

    for (s32 i = 0 ; i < 8; ++i) {
	ssize alignment = 1 << i;
	s32 *arr = la_allocate(&arena, 1, SIZEOF(s32), alignment);
	REQUIRE(is_aligned((ssize)arr, alignment));
    }

    la_destroy(&arena);
}

TEST_CASE(linear_arena_alloc_larger_than_buffer)
{
    LinearArena arena = la_create(default_allocator, 1024);

    la_allocate(&arena, 3000, 1, ALIGNOF(s32));
    REQUIRE(la_get_memory_usage(&arena) >= 3000);

    la_reset(&arena);
    REQUIRE(la_get_memory_usage(&arena) == 0);

    la_destroy(&arena);
}

TEST_CASE(linear_arena_alloc_exceed_buffer)
{
    LinearArena arena = la_create(default_allocator, 1024);

    void *first = la_allocate(&arena, 4, 1, ALIGNOF(s32));
    void *second = la_allocate(&arena, 3000, 1, ALIGNOF(s32));
    REQUIRE(first != second);
    REQUIRE(la_get_memory_usage(&arena) >= 3004);


    la_destroy(&arena);
}

TEST_CASE(linear_arena_alloc_exceed_buffer_slightly)
{
    const s32 size = 1024;
    LinearArena arena = la_create(default_allocator, size);

    la_allocate_array(&arena, byte, size / 2);
    la_allocate_array(&arena, byte, size / 2 + 1);

    REQUIRE(la_get_memory_usage(&arena) == size + size / 2 + 1);

    la_destroy(&arena);
}

TEST_CASE(linear_arena_alloc_exceed_buffer_multiple)
{
    LinearArena arena = la_create(default_allocator, 1024);

    ssize alloc_count = 3000;
    for (s32 i = 0; i < alloc_count; ++i) {
	s32 *alloc = la_allocate_item(&arena, s32);
	REQUIRE(is_aligned((ssize)alloc, ALIGNOF(s32)));
    }

    REQUIRE(la_get_memory_usage(&arena) >= (alloc_count * SIZEOF(s32)));

    la_destroy(&arena);
}

TEST_CASE(linear_arena_reset_basic)
{
    LinearArena arena = la_create(default_allocator, 1024);

    s32 *first = la_allocate(&arena, 10, SIZEOF(s32), ALIGNOF(s32));

    la_reset(&arena);
    REQUIRE(la_get_memory_usage(&arena) == 0);

    s32 *second = la_allocate(&arena, 10, SIZEOF(s32), ALIGNOF(s32));

    REQUIRE(first == second);

    la_destroy(&arena);
}

TEST_CASE(linear_arena_reset_multi_buffers)
{
    LinearArena arena = la_create(default_allocator, 1024);

    byte *first = la_allocate_array(&arena, byte, 1);
    byte *second = la_allocate_array(&arena, byte, 1);
    la_allocate_array(&arena, byte, 3000);

    la_reset(&arena);

    byte *last = la_allocate_array(&arena, byte, 1);
    REQUIRE(last == first);

    last = la_allocate_array(&arena, byte, 1);
    REQUIRE(last == second);

    la_destroy(&arena);
}

TEST_CASE(linear_arena_write_adjacent_first)
{
    LinearArena arena = la_create(default_allocator, 1024);

    ssize alloc_size = 16;
    byte *first = la_allocate_array(&arena, byte, alloc_size);
    byte *second = la_allocate_array(&arena, byte, alloc_size);

    memset(first, 0xDE, (usize)alloc_size);
    memset(second, 0xFD, (usize)alloc_size);

    for (ssize i = 0; i < alloc_size; ++i) {
	REQUIRE(first[i] == 0xDE);
	REQUIRE(second[i] == 0xFD);
    }

    la_destroy(&arena);
}

TEST_CASE(linear_arena_write_adjacent_second)
{
    LinearArena arena = la_create(default_allocator, 1024);

    ssize alloc_size = 16;
    byte *first = la_allocate_array(&arena, byte, alloc_size);
    byte *second = la_allocate_array(&arena, byte, alloc_size);

    memset(second, 0xFD, (usize)alloc_size);
    memset(first, 0xDE, (usize)alloc_size);

    for (ssize i = 0; i < alloc_size; ++i) {
	REQUIRE(first[i] == 0xDE);
	REQUIRE(second[i] == 0xFD);
    }

    la_destroy(&arena);
}

TEST_CASE(linear_arena_new_allocation_zeroed)
{
    LinearArena arena = la_create(default_allocator, 1024);
    byte *arr = la_allocate_array(&arena, byte, 16);

    for (s32 i = 0; i < 16; ++i) {
        REQUIRE(arr[i] == 0);
    }

    la_destroy(&arena);
}

TEST_CASE(linear_arena_zeroed_memory_after_reset)
{
     LinearArena arena = la_create(default_allocator, 1024);

     ssize alloc_size = 32;
     byte *first = la_allocate_array(&arena, byte, alloc_size);
     memset(first, 0xCD, (usize)alloc_size);

     la_reset(&arena);

     byte *second = la_allocate_array(&arena, byte, alloc_size);
     REQUIRE(first == second);

     byte *expected = malloc((usize)alloc_size);
     memset(expected, 0, (usize)alloc_size);

     REQUIRE(memcmp(second, expected, (usize)alloc_size) == 0);

     free(expected);

     la_destroy(&arena);
}

TEST_CASE(linear_arena_copy_allocation)
{
     LinearArena arena = la_create(default_allocator, 1024);

     ssize alloc_size = 32;
     s32 *first = la_allocate_array(&arena, s32, alloc_size);
     memset(first, 0xCD, (usize)alloc_size * sizeof(*first));

     s32 *second = la_copy_array(&arena, first, alloc_size);

     REQUIRE(is_aligned((ssize)second, ALIGNOF(s32)));
     REQUIRE(first != second);
     REQUIRE(memcmp(first, second, (usize)alloc_size * sizeof(s32)) == 0);


     la_destroy(&arena);
}

TEST_CASE(linear_arena_meet_capacity_exactly)
{
    const s32 size = 1024;
    LinearArena arena = la_create(default_allocator, size);

    byte *arr1 = la_allocate_array(&arena, byte, size / 2);
    byte *arr2 = la_allocate_array(&arena, byte, size / 2);

    REQUIRE(arr1);
    REQUIRE(arr2);
    REQUIRE(la_get_memory_usage(&arena) == size);

    la_destroy(&arena);
}

TEST_CASE(linear_arena_pop_to_basic)
{
    LinearArena arena = la_create(default_allocator, 1024);
    byte *first = la_allocate_array(&arena, byte, 256);

    la_pop_to(&arena, first);

    byte *next = la_allocate_array(&arena, byte, 256);
    REQUIRE(next == first);

    la_destroy(&arena);
}

TEST_CASE(linear_arena_pop_to_across_buffers)
{
    LinearArena arena = la_create(default_allocator, 1024);
    byte *first = la_allocate_array(&arena, byte, 256);
    byte *second = la_allocate_array(&arena, byte, 1000);

    la_pop_to(&arena, second);

    byte *third = la_allocate_array(&arena, byte, 10);
    REQUIRE(third == second);

    la_pop_to(&arena, first);
    byte *fourth = la_allocate_array(&arena, byte, 10);
    REQUIRE(fourth == first);

    la_destroy(&arena);
}

TEST_CASE(linear_arena_copy_array)
{
    LinearArena arena = la_create(default_allocator, 1024);
    s32 *first = la_allocate_array(&arena, s32, 4);

    for (s32 i = 0; i < 4; ++i) {
        first[i] = i;
    }

    s32 *second = la_copy_array(&arena, first, 4);

    REQUIRE(first != second);
    REQUIRE(is_aligned((ssize)second, 4));

    REQUIRE(memcmp(first, second, 4 * sizeof(*first)) == 0);

    la_destroy(&arena);
}
