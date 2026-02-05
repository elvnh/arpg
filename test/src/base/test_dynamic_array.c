#include "test_macros.h"
#include "base/dynamic_array.h"


typedef struct {
    int *items;
    ssize count;
    ssize capacity;
} IntArray;

TEST_CASE(da_push_without_init)
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator alloc = la_allocator(&arena);

    IntArray arr = {0};

    for (int i = 0; i < 10; ++i) {
        da_push(&arr, i, alloc);
    }

    for (ssize i = 0; i < 10; ++i) {
        REQUIRE(arr.items[i] == i);
    }

    la_destroy(&arena);
}

TEST_CASE(da_push_with_init)
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator alloc = la_allocator(&arena);

    ssize cap = 4;
    IntArray arr;
    da_init(&arr, cap, alloc);

    void *old_items = arr.items;

    for (int i = 0; i < cap; ++i) {
        da_push(&arr, i, alloc);
    }

    REQUIRE(arr.items == old_items);

    la_destroy(&arena);
}

TEST_CASE(da_push_until_realloc)
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator alloc = la_allocator(&arena);

    ssize cap = 4;
    IntArray arr;
    da_init(&arr, cap, alloc);

    void *old_items = arr.items;

    for (int i = 0; i < cap * 8; ++i) {
        da_push(&arr, i, alloc);
    }

    REQUIRE(arr.items != old_items);

    for (int i = 0; i < cap * 8; ++i) {
        REQUIRE(arr.items[i] == i);
    }

    la_destroy(&arena);
}
