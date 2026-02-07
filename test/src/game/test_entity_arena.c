#include "base/utils.h"
#include "entity/entity_arena.h"
#include "test_macros.h"
#include "testing_utils.h"

TEST_CASE(entity_arena_basic)
{
    EntityArena arena = {0};
    REQUIRE(entity_arena_get_memory_usage(&arena) == 0);
    REQUIRE(entity_arena_get_free_memory(&arena) == ENTITY_ARENA_SIZE);
}


TEST_CASE(entity_arena_alloc_basic)
{
    EntityArena arena = {0};

    EntityArenaAllocation alloc = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int));
    int *ptr = alloc.ptr;

    REQUIRE(alloc.index.offset == 0);
    REQUIRE(entity_arena_get_memory_usage(&arena) == SIZEOF(int));
    REQUIRE(entity_arena_get_free_memory(&arena) == ENTITY_ARENA_SIZE - SIZEOF(int));

    REQUIRE(is_aligned((ssize)ptr, ALIGNOF(int)));
}

TEST_CASE(entity_arena_alloc_zeroed)
{
    EntityArena arena = {0};
    arena.data[0] = 'A';

    EntityArenaAllocation alloc = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int));
    int *ptr = alloc.ptr;

    REQUIRE(is_zeroed(ptr, SIZEOF(*ptr)));
}

TEST_CASE(entity_arena_adjacent_writes_1)
{
    EntityArena arena = {0};

    EntityArenaAllocation alloc1 = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int));
    EntityArenaAllocation alloc2 = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int));

    int *ptr1 = alloc1.ptr;
    int *ptr2 = alloc2.ptr;

    *ptr1 = 123;
    *ptr2 = 456;

    REQUIRE(*ptr1 == 123);
}

TEST_CASE(entity_arena_adjacent_writes_2)
{
    EntityArena arena = {0};

    EntityArenaAllocation alloc1 = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int));
    EntityArenaAllocation alloc2 = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int));

    int *ptr1 = alloc1.ptr;
    int *ptr2 = alloc2.ptr;

    *ptr2 = 456;
    *ptr1 = 123;

    REQUIRE(*ptr2 == 456);
}

TEST_CASE(entity_arena_alloc_to_cap_in_one_alloc)
{
    EntityArena arena = {0};

    entity_arena_allocate(&arena, ENTITY_ARENA_SIZE, 1);

    REQUIRE(entity_arena_get_free_memory(&arena) == 0);
    REQUIRE(entity_arena_get_memory_usage(&arena) == ENTITY_ARENA_SIZE);
}

TEST_CASE(entity_arena_alloc_to_cap_multiple)
{
    EntityArena arena = {0};

    ssize n = 4;
    ssize alloc_size = ENTITY_ARENA_SIZE / n;

    for (ssize i = 0; i < n; ++i) {
	entity_arena_allocate(&arena, alloc_size, 1);
    }

    REQUIRE(entity_arena_get_free_memory(&arena) == 0);
    REQUIRE(entity_arena_get_memory_usage(&arena) == ENTITY_ARENA_SIZE);
}

TEST_CASE(entity_arena_alignment)
{
    EntityArena arena = {0};

    for (ssize i = 0; i < 7; ++i) {
	ssize align = 1u << i;

	EntityArenaAllocation alloc = entity_arena_allocate(&arena, 1, align);
	REQUIRE(is_aligned((ssize)alloc.ptr, align));

    }
}

TEST_CASE(entity_arena_get)
{
    EntityArena arena = {0};

    for (ssize i = 0; i < 10; ++i) {
	EntityArenaAllocation alloc = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int));

	int *ptr = entity_arena_get(&arena, alloc.index);
	REQUIRE(ptr == alloc.ptr);
    }
}

TEST_CASE(entity_arena_reset)
{
    EntityArena arena = {0};

    void *first = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int)).ptr;

    entity_arena_reset(&arena);
    REQUIRE(entity_arena_get_memory_usage(&arena) == 0);
    REQUIRE(entity_arena_get_free_memory(&arena) == ENTITY_ARENA_SIZE);

    void *second = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int)).ptr;
    REQUIRE(first == second);

}

TEST_CASE(entity_arena_zeroed_after_reset)
{
    EntityArena arena = {0};

    int *first = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int)).ptr;
    *first = 0xCD;

    entity_arena_reset(&arena);

    int *second = entity_arena_allocate(&arena, SIZEOF(int), ALIGNOF(int)).ptr;
    REQUIRE(first == second);
    REQUIRE(*second == 0);
}
