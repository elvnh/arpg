#include "test_macros.h"
#include "base/ring_buffer.h"

#define STATIC_INT_BUFFER_CAP 3

DEFINE_STATIC_RING_BUFFER(int, StaticIntBuffer, STATIC_INT_BUFFER_CAP);

TEST_CASE(ring_init_static)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);
    REQUIRE(buf.capacity == STATIC_INT_BUFFER_CAP);
    REQUIRE(ring_is_empty(&buf));
    REQUIRE(!ring_is_full(&buf));
    REQUIRE(ring_length(&buf) == 0);
}

TEST_CASE(ring_init_heap)
{
    LinearArena arena = la_create(default_allocator, 4096);
    Allocator allocator = la_allocator(&arena);

    DEFINE_HEAP_RING_BUFFER(int, HeapIntBuffer);

    {
        HeapIntBuffer buf;
        ring_initialize(&buf, 4, allocator);
        REQUIRE(buf.capacity == 4);
        REQUIRE(buf.items);
        REQUIRE(ring_is_empty(&buf));
    }

    la_destroy(&arena);
}

TEST_CASE(ring_push_buf_basic)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    // TODO: pushing items by value
    s32 i = 123;
    ring_push(&buf, &i);

    REQUIRE(ring_length(&buf) == 1);
    REQUIRE(!ring_is_empty(&buf));
    REQUIRE(!ring_is_full(&buf));
    REQUIRE(*ring_at(&buf, 0) == 123);
    REQUIRE(*ring_peek(&buf) == 123);
    REQUIRE(*ring_peek_tail(&buf) == 123);

    ring_pop(&buf);
    REQUIRE(ring_length(&buf) == 0);
    REQUIRE(ring_is_empty(&buf));
    REQUIRE(!ring_is_full(&buf));
}

TEST_CASE(ring_pop_basic)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    s32 i = 123;
    ring_push(&buf, &i);

    ring_pop(&buf);
    REQUIRE(ring_length(&buf) == 0);
    REQUIRE(ring_is_empty(&buf));
    REQUIRE(!ring_is_full(&buf));
}

TEST_CASE(ring_push_to_cap)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity; ++i) {
        ring_push(&buf, &i);
        REQUIRE(ring_length(&buf) == (i + 1));
    }

    REQUIRE(ring_length(&buf) == buf.capacity);
    REQUIRE(ring_is_full(&buf));
    REQUIRE(!ring_is_empty(&buf));

    for (s32 i = 0; i < ring_length(&buf); ++i) {
        REQUIRE(*ring_at(&buf, i) == i);
    }
}

TEST_CASE(ring_pop_from_full_cap)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity; ++i) {
        ring_push(&buf, &i);
    }

    ring_pop(&buf);
    REQUIRE(ring_length(&buf) == buf.capacity - 1);
    REQUIRE(*ring_peek(&buf) == 1);

    ring_pop(&buf);
    REQUIRE(*ring_peek(&buf) == 2);

    ring_pop(&buf);
    REQUIRE(ring_is_empty(&buf));
}

TEST_CASE(ring_pop_until_empty_then_push)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity; ++i) {
        ring_push(&buf, &i);
    }

    for (s32 i = 0; i < buf.capacity; ++i) {
        ring_pop(&buf);
    }

    REQUIRE(ring_is_empty(&buf));

    s32 i = 0;
    ring_push(&buf, &i);

    REQUIRE(ring_length(&buf) == 1);
}

TEST_CASE(ring_pop_next_to_last_element)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    s32 i = 0;
    ring_push(&buf, &i);

    i = 1;
    ring_push(&buf, &i);

    REQUIRE(ring_length(&buf) == 2);
    REQUIRE(*ring_at(&buf, 0) == 0);
    REQUIRE(*ring_at(&buf, 1) == 1);

    ring_pop(&buf);
    REQUIRE(ring_length(&buf) == 1);
    REQUIRE(*ring_at(&buf, 0) == 1);
}

TEST_CASE(ring_push_and_pop)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    s32 i = 0;
    ring_push(&buf, &i);

    i = 1;
    ring_push(&buf, &i);

    ring_pop(&buf);

    i = 2;
    ring_push(&buf, &i);

    i = 3;
    ring_push(&buf, &i);

    REQUIRE(*ring_at(&buf, 0) == 1);
    REQUIRE(*ring_at(&buf, 1) == 2);
    REQUIRE(*ring_at(&buf, 2) == 3);

    ring_pop(&buf);
    ring_pop(&buf);
    ring_pop(&buf);

    REQUIRE(ring_is_empty(&buf));
}

TEST_CASE(ring_push_overwrite)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity * 2; ++i) {
        ring_push_overwrite(&buf, &i);
    }

    REQUIRE(ring_is_full(&buf));

    for (s32 i = 0; i < ring_length(&buf); ++i) {
        REQUIRE(*ring_at(&buf, i) == (i + buf.capacity));
    }
}

TEST_CASE(ring_peek_and_pop_tail)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity; ++i) {
        ring_push(&buf, &i);
    }

    REQUIRE(*ring_peek_tail(&buf) == 2);
    ring_pop_tail(&buf);

    REQUIRE(*ring_peek_tail(&buf) == 1);
    ring_pop_tail(&buf);

    REQUIRE(*ring_peek_tail(&buf) == 0);
    ring_pop_tail(&buf);

    REQUIRE(ring_is_empty(&buf));
}

TEST_CASE(ring_swap_remove_first)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity; ++i) {
        ring_push(&buf, &i);
    }

    ring_swap_remove(&buf, 0);
    REQUIRE(ring_length(&buf) == 2);

    REQUIRE(*ring_at(&buf, 0) == 2);
    REQUIRE(*ring_at(&buf, 1) == 1);
}

TEST_CASE(ring_swap_remove_middle)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity; ++i) {
        ring_push(&buf, &i);
    }

    ring_swap_remove(&buf, 1);
    REQUIRE(ring_length(&buf) == 2);

    REQUIRE(*ring_at(&buf, 0) == 0);
    REQUIRE(*ring_at(&buf, 1) == 2);
}

TEST_CASE(ring_swap_remove_last)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity; ++i) {
        ring_push(&buf, &i);
    }

    ring_swap_remove(&buf, 2);
    REQUIRE(ring_length(&buf) == 2);

    REQUIRE(*ring_at(&buf, 0) == 0);
    REQUIRE(*ring_at(&buf, 1) == 1);
}

TEST_CASE(ring_swap_remove_until_empty)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity; ++i) {
        ring_push(&buf, &i);
    }

    ring_swap_remove(&buf, 2);
    ring_swap_remove(&buf, 1);

    REQUIRE(ring_length(&buf) == 1);
    REQUIRE(*ring_peek(&buf) == 0);

    ring_swap_remove(&buf, 0);

    REQUIRE(ring_is_empty(&buf));
}

TEST_CASE(ring_pop_load)
{
    StaticIntBuffer buf;
    ring_initialize_static(&buf);

    for (s32 i = 0; i < buf.capacity; ++i) {
        s32 j = i + 10;
        ring_push(&buf, &j);
    }

    REQUIRE(ring_pop_load(&buf) == 10);
    REQUIRE(ring_pop_load(&buf) == 11);
    REQUIRE(ring_pop_load(&buf) == 12);

}
