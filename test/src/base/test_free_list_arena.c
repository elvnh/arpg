#include "test_macros.h"
#include "base/free_list_arena.h"
#include "testing_utils.h"

TEST_CASE(fl_create)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    REQUIRE(fl_get_available_memory(&fl) == 1024);
    REQUIRE(fl_get_memory_usage(&fl) == 0);

    fl_destroy(&fl);
}

TEST_CASE(fl_alloc_basic)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    fl_allocate(&fl, 1, 100, 4);
    REQUIRE((fl_get_memory_usage(&fl) >= 100) && (fl_get_memory_usage(&fl) < 200));

    fl_destroy(&fl);
}

TEST_CASE(fl_alloc_zeroed)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *p = fl_allocate(&fl, 1, 100, 4);
    REQUIRE(is_zeroed(p, 100));

    fl_destroy(&fl);
}

TEST_CASE(fl_alloc_zeroed_after_free)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *p = fl_allocate(&fl, 1, 100, 4);
    memset(p, 0xCD, 100);

    fl_deallocate(&fl, p);

    byte *p1 = fl_allocate(&fl, 1, 100, 4);
    REQUIRE(p == p1);
    REQUIRE(is_zeroed(p1, 100));

    fl_destroy(&fl);
}


TEST_CASE(fl_alloc_close_to_cap)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    fl_allocate(&fl, 1, 100, 4);
    fl_allocate(&fl, 1, 880, 4);

    REQUIRE((fl_get_memory_usage(&fl) >= 980) && (fl_get_memory_usage(&fl) <= 1024));
    REQUIRE((fl_get_available_memory(&fl) == 0));

    fl_destroy(&fl);
}

TEST_CASE(fl_alloc_over_cap)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    void *p1 = fl_allocate(&fl, 900, 1, 4);
    REQUIRE(fl_get_memory_usage(&fl) >= 900 && fl_get_memory_usage(&fl) <= 1024);
    REQUIRE(fl_get_available_memory(&fl) <= 124);

    void *p2 = fl_allocate(&fl, 900, 1, 4);
    REQUIRE(fl_get_memory_usage(&fl) >= 1800 && fl_get_memory_usage(&fl) <= 2048);

    fl_deallocate(&fl, p1);
    fl_deallocate(&fl, p2);

    REQUIRE(fl_get_memory_usage(&fl) == 0);

    fl_destroy(&fl);
}

TEST_CASE(fl_dealloc_one)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *p = fl_allocate(&fl, 1, 100, 4);

    fl_deallocate(&fl, p);

    REQUIRE(fl_get_available_memory(&fl) == 1024);
    REQUIRE(fl_get_memory_usage(&fl) == 0);

    fl_destroy(&fl);
}

TEST_CASE(fl_dealloc_two)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *p = fl_allocate(&fl, 1, 100, 4);
    byte *p2 = fl_allocate(&fl, 1, 880, 4);

    fl_deallocate(&fl, p);
    fl_deallocate(&fl, p2);

    REQUIRE(fl_get_available_memory(&fl) == 1024);
    REQUIRE(fl_get_memory_usage(&fl) == 0);

    fl_destroy(&fl);
}

TEST_CASE(fl_alloc_free_return_same_pointer)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *p1 = fl_allocate(&fl, 1, 100, 4);
    fl_deallocate(&fl, p1);

    byte *p2 = fl_allocate(&fl, 1, 200, 4);
    REQUIRE(p1 == p2);

    fl_destroy(&fl);
}

TEST_CASE(fl_three_allocs_free_in_order)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *p1 = fl_allocate(&fl, 1, 100, 4);
    byte *p2 = fl_allocate(&fl, 1, 100, 4);
    byte *p3 = fl_allocate(&fl, 1, 100, 4);

    REQUIRE(fl_get_available_memory(&fl) <= 724);
    REQUIRE(fl_get_memory_usage(&fl) >= 300);

    fl_deallocate(&fl, p1);
    fl_deallocate(&fl, p2);
    fl_deallocate(&fl, p3);

    REQUIRE(fl_get_memory_usage(&fl) == 0);
    REQUIRE(fl_get_available_memory(&fl) == 1024);

    fl_destroy(&fl);
}

TEST_CASE(fl_three_allocs_free_middle)
{
    FreeListArena fl = fl_create(default_allocator, 1024);
    byte *p1 = fl_allocate(&fl, 1, 100, 4);
    byte *p2 = fl_allocate(&fl, 1, 100, 4);
    byte *p3 = fl_allocate(&fl, 1, 100, 4);

    fl_deallocate(&fl, p2);
    fl_deallocate(&fl, p1);
    fl_deallocate(&fl, p3);

    REQUIRE(fl_get_memory_usage(&fl) == 0);
    REQUIRE(fl_get_available_memory(&fl) == 1024);

    fl_destroy(&fl);
}

TEST_CASE(fl_three_allocs_free_last_first)
{
    FreeListArena fl = fl_create(default_allocator, 1024);
    byte *p1 = fl_allocate(&fl, 1, 100, 4);
    byte *p2 = fl_allocate(&fl, 1, 100, 4);
    byte *p3 = fl_allocate(&fl, 1, 100, 4);

    fl_deallocate(&fl, p3);
    fl_deallocate(&fl, p1);
    fl_deallocate(&fl, p2);

    REQUIRE(fl_get_available_memory(&fl) == 1024);

    fl_destroy(&fl);
}

TEST_CASE(fl_three_allocs_free_reverse_order)
{
    FreeListArena fl = fl_create(default_allocator, 1024);
    byte *p1 = fl_allocate(&fl, 1, 100, 4);
    byte *p2 = fl_allocate(&fl, 1, 100, 4);
    byte *p3 = fl_allocate(&fl, 1, 100, 4);

    fl_deallocate(&fl, p3);
    fl_deallocate(&fl, p2);
    fl_deallocate(&fl, p1);

    REQUIRE(fl_get_memory_usage(&fl) == 0);
    REQUIRE(fl_get_available_memory(&fl) == 1024);

    fl_destroy(&fl);
}

TEST_CASE(fl_three_allocs_free_1_2_0)
{
    FreeListArena fl = fl_create(default_allocator, 1024);
    byte *p1 = fl_allocate(&fl, 1, 100, 4);
    byte *p2 = fl_allocate(&fl, 1, 100, 4);
    byte *p3 = fl_allocate(&fl, 1, 100, 4);

    fl_deallocate(&fl, p2);
    fl_deallocate(&fl, p3);
    fl_deallocate(&fl, p1);

    REQUIRE(fl_get_memory_usage(&fl) == 0);
    REQUIRE(fl_get_available_memory(&fl) == 1024);

    fl_destroy(&fl);
}

TEST_CASE(fl_three_allocs_free_0_2_1)
{
    FreeListArena fl = fl_create(default_allocator, 1024);
    byte *p1 = fl_allocate(&fl, 1, 100, 4);
    byte *p2 = fl_allocate(&fl, 1, 100, 4);
    byte *p3 = fl_allocate(&fl, 1, 100, 4);

    fl_deallocate(&fl, p1);
    fl_deallocate(&fl, p3);
    fl_deallocate(&fl, p2);

    REQUIRE(fl_get_memory_usage(&fl) == 0);
    REQUIRE(fl_get_available_memory(&fl) == 1024);

    fl_destroy(&fl);
}

TEST_CASE(fl_alignment)
{
    FreeListArena fl = fl_create(default_allocator, MB(32));

    for (s32 i = 1; i <= 16; ++i) {
        s32 alignment = 1 << i;

        byte *p = fl_allocate(&fl, 1, 100, alignment);

        REQUIRE(is_aligned((ssize)p, alignment));
    }

    fl_destroy(&fl);
}

TEST_CASE(fl_adjacent_write_first)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    ssize alloc_size = 16;
    byte *first = fl_alloc_array(&fl, byte, alloc_size);
    byte *second = fl_alloc_array(&fl, byte, alloc_size);

    memset(first, 0xDE, (usize)alloc_size);
    memset(second, 0xFD, (usize)alloc_size);

    for (ssize i = 0; i < alloc_size; ++i) {
	REQUIRE(first[i] == 0xDE);
	REQUIRE(second[i] == 0xFD);
    }

    fl_destroy(&fl);
}

TEST_CASE(fl_adjacent_write_second)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    ssize alloc_size = 16;
    byte *first = fl_alloc_array(&fl, byte, alloc_size);
    byte *second = fl_alloc_array(&fl, byte, alloc_size);

    memset(second, 0xFD, (usize)alloc_size);
    memset(first, 0xDE, (usize)alloc_size);

    for (ssize i = 0; i < alloc_size; ++i) {
	REQUIRE(first[i] == 0xDE);
	REQUIRE(second[i] == 0xFD);
    }

    fl_destroy(&fl);
}


TEST_CASE(fl_alloc_many)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *p1 = fl_allocate(&fl, 1, 250, 4);
    byte *p2 = fl_allocate(&fl, 1, 250, 4);
    byte *p3 = fl_allocate(&fl, 1, 250, 4);

    byte *p4 = fl_allocate(&fl, 1, 250, 4);
    byte *p5 = fl_allocate(&fl, 1, 250, 4);
    byte *p6 = fl_allocate(&fl, 1, 250, 4);

    fl_deallocate(&fl, p1);
    fl_deallocate(&fl, p3);
    fl_deallocate(&fl, p2);

    REQUIRE(fl_get_available_memory(&fl) >= 1024 && fl_get_available_memory(&fl) < 2048);

    fl_deallocate(&fl, p4);
    fl_deallocate(&fl, p6);
    fl_deallocate(&fl, p5);

    REQUIRE(fl_get_available_memory(&fl) == 2048);
    REQUIRE(fl_get_memory_usage(&fl) == 0);

    fl_destroy(&fl);
}

TEST_CASE(fl_realloc_by_memcpy)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *ptr1 = fl_allocate(&fl, 100, sizeof(byte), 4);
    byte *ptr2 = fl_allocate(&fl, 100, sizeof(byte), 4);

    memset(ptr1, 0xCD, 100 * sizeof(byte));
    byte *new_ptr1 = fl_reallocate(&fl, ptr1, 200, sizeof(byte), 4);

    REQUIRE(new_ptr1 != ptr1);

    for (s32 i = 0; i < 100; ++i) {
        REQUIRE(new_ptr1[i] == 0xCD);
    }

    fl_deallocate(&fl, new_ptr1);
    fl_deallocate(&fl, ptr2);

    REQUIRE(fl_get_memory_usage(&fl) == 0);
    REQUIRE(fl_get_available_memory(&fl) == 1024);


    fl_destroy(&fl);
}

TEST_CASE(fl_realloc_in_place)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *ptr1 = fl_allocate(&fl, 100, sizeof(byte), 4);
    byte *new_ptr1 = fl_reallocate(&fl, ptr1, 200, sizeof(byte), 4);

    REQUIRE(new_ptr1 == ptr1);
    REQUIRE(fl_get_memory_usage(&fl) >= 200);

    fl_destroy(&fl);
}


TEST_CASE(fl_realloc_larger_than_buffer)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    byte *ptr1 = fl_allocate(&fl, 1, 100, 4);
    byte *ptr2 = fl_allocate(&fl, 1, 100, 4);

    byte *new_ptr1 = fl_reallocate(&fl, ptr1, 100, 200, 4);

    REQUIRE(new_ptr1);
    REQUIRE(new_ptr1 != ptr1);

    fl_deallocate(&fl, new_ptr1);
    fl_deallocate(&fl, ptr2);

    REQUIRE(fl_get_memory_usage(&fl) == 0);

    fl_destroy(&fl);
}

TEST_CASE(fl_realloc_to_smaller_size_many)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    for (s32 size = 2; size <= 100; ++size) {
        byte *ptr1 = fl_allocate(&fl, size, 1, 4);
        byte *new_ptr1 = fl_reallocate(&fl, ptr1, size / 2, 1, 4);

        REQUIRE(new_ptr1);
        REQUIRE(new_ptr1 == ptr1);

        fl_deallocate(&fl, new_ptr1);

        REQUIRE(fl_get_memory_usage(&fl) == 0);
    }

    fl_destroy(&fl);
}

TEST_CASE(fl_realloc_to_greater_or_equal_size_many)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    for (s32 size = 100; size <= 500; ++size) {
        byte *ptr1 = fl_allocate(&fl, 100, 1, 4);
        byte *new_ptr1 = fl_reallocate(&fl, ptr1, size, 1, 4);

        REQUIRE(new_ptr1);
        REQUIRE(new_ptr1 == ptr1);

        fl_deallocate(&fl, new_ptr1);

        REQUIRE(fl_get_memory_usage(&fl) == 0);
    }

    fl_destroy(&fl);
}

TEST_CASE(fl_realloc_array_in_place)
{
    FreeListArena fl = fl_create(default_allocator, 1024);
    byte *ptr = fl_alloc_array(&fl, byte, 50);
    byte *ptr1 = fl_realloc_array(&fl, byte, ptr, 100);
    REQUIRE(ptr1 == ptr);

    REQUIRE(fl_get_memory_usage(&fl) >= 100);

    fl_destroy(&fl);
}

TEST_CASE(fl_realloc_array_by_memcpy)
{
    FreeListArena fl = fl_create(default_allocator, 4096);
    ssize original_size = 50;

    byte *ptr = fl_alloc_array(&fl, byte, original_size);
    memset(ptr, 0xCD, (usize)original_size);

    fl_alloc_item(&fl, byte);
    byte *ptr1 = fl_realloc_array(&fl, byte, ptr, original_size * 2);
    REQUIRE(ptr1 != ptr);

    for (s32 i = 0; i < original_size; ++i) {
        REQUIRE(ptr1[i] == 0xCD);
    }

    fl_destroy(&fl);
}

TEST_CASE(fl_realloc_to_smaller)
{
    FreeListArena fl = fl_create(default_allocator, 1024);
    byte *ptr = fl_alloc_array(&fl, byte, 100);
    ssize max_usage = fl_get_memory_usage(&fl);

    byte *ptr1 = fl_realloc_array(&fl, byte, ptr, 50);

    REQUIRE(ptr1 == ptr);
    REQUIRE(fl_get_memory_usage(&fl) >= 50);
    REQUIRE(fl_get_memory_usage(&fl) < max_usage);

    fl_destroy(&fl);
}

TEST_CASE(fl_reset_empty)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    fl_reset(&fl);
    REQUIRE(fl_get_memory_usage(&fl) == 0);

    fl_destroy(&fl);
}

TEST_CASE(fl_reset_non_empty)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    fl_alloc_item(&fl, s32);

    fl_reset(&fl);
    REQUIRE(fl_get_memory_usage(&fl) == 0);

    fl_destroy(&fl);
}

TEST_CASE(fl_reset_many_times)
{
    FreeListArena fl = fl_create(default_allocator, 1024);

    for (s32 i = 0; i < 3; ++i) {
        for (s32 j = 0; j < 100; ++j) {
            fl_alloc_array(&fl, byte, 100);
        }

        fl_reset(&fl);
        REQUIRE(fl_get_memory_usage(&fl) == 0);
    }

    fl_destroy(&fl);
}
