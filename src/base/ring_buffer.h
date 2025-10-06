#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "base/utils.h"

/*
  TODO:
  - Return value when popping, can probably be done with comma operator
  - If using comma operator to bounds check, can't take address of expression.
    Return address?
  - Swap remove
  - Unit tests
 */

#define DEFINE_RING_BUFFER(type, name)          \
    typedef struct name {                       \
        type  *items;                           \
        ssize  head;                            \
        ssize  tail;                            \
        ssize  capacity;                        \
    } name

#define ring_initialize(buf, cap, allocator)                        \
    do {                                                                \
        ASSERT(!multiply_overflows_ssize((cap), SIZEOF(*(buf)->items))); \
        (buf)->items = allocate(allocator, (cap) * SIZEOF(*(buf)->items)); \
        ring_impl_set_to_empty(&(buf)->head, &(buf)->tail);         \
        (buf)->capacity = (cap);                                        \
    } while (0)

#define ring_push(buf, item) ring_push_impl((buf)->items, &(buf)->head, \
        &(buf)->tail, (buf)->capacity, SIZEOF(*(buf)->items), (item))
#define ring_push_overwrite(buf, item) ring_push_overwrite_impl((buf)->items, \
        &(buf)->head, &(buf)->tail, (buf)->capacity, SIZEOF(*(buf)->items), (item))
#define ring_pop(buf) ring_pop_impl((buf)->items, &(buf)->head, &(buf)->tail, \
        (buf)->capacity, SIZEOF(*((buf)->items)))
#define ring_pop_tail(buf) ring_pop_tail_impl((buf)->items, &(buf)->head, &(buf)->tail, \
        (buf)->capacity, SIZEOF(*((buf)->items)))

#define ring_at(buf, idx) (ring_internal_bounds_check((idx), ring_length((buf))), \
        &((buf)->items[((buf)->head + (idx)) % (buf)->capacity]))
#define ring_peek(buf) ring_at((buf), 0)

#define ring_length(buf) ring_length_impl((buf)->head, (buf)->tail, (buf)->capacity)
#define ring_is_full(buf) ring_is_full_impl(&(buf)->head, &(buf)->tail)
#define ring_is_empty(buf) ring_is_empty_impl(&(buf)->head, &(buf)->tail)

DEFINE_RING_BUFFER(int, IntBuffer);

static inline b32 ring_is_empty_impl(ssize *head, ssize *tail)
{
    b32 result = *head == -1;
    ASSERT(!result || (*tail == -1));

    return result;
}

static inline b32 ring_is_full_impl(ssize *head, ssize *tail)
{
    b32 result = !ring_is_empty_impl(head, tail) && (*head == *tail);
    return result;
}

static inline void ring_impl_set_to_empty(ssize *head, ssize *tail)
{
    *head = -1;
    *tail = -1;
}

static inline void *ring_pop_impl(void *items, ssize *head, ssize *tail, ssize capacity, ssize item_size)
{
    ASSERT(*head != -1);
    void *result = byte_offset(items, *head * item_size);

    ssize new_head = (*head + 1) % capacity;

    if (new_head == *tail) {
        ring_impl_set_to_empty(head, tail);
    } else {
        *head = new_head;
    }

    return result;
}

static inline void *ring_pop_tail_impl(void *items, ssize *head, ssize *tail, ssize capacity, ssize item_size)
{
    ASSERT(!ring_is_empty_impl(head, tail));

    *tail -= 1;

    if (*tail == -1) {
        *tail = capacity - 1;
    }

    void *result = byte_offset(items, *tail * item_size);

    if (*tail == *head) {
        ring_impl_set_to_empty(head, tail);
    }

    return result;
}

static inline void ring_push_impl(void *items, ssize *head, ssize *tail, ssize capacity, ssize item_size, void *item)
{
    ASSERT((*head != *tail) || ((*head == -1) && (*tail == -1)));

    if (*head == -1) {
        *head = 0;
        *tail = 0;
    }

    memcpy(byte_offset(items, *tail * item_size), item, cast_ssize_to_usize(item_size));
    *tail = (*tail + 1) % capacity;
}

static inline void ring_push_overwrite_impl(void *items, ssize *head, ssize *tail, ssize capacity,
    ssize item_size, void *item)
{
    // TODO: this can be made more efficient
    if (ring_is_full_impl(head, tail)) {
        ring_pop_impl(items, head, tail, capacity, item_size);
    }

    ring_push_impl(items, head, tail, capacity, item_size, item);
}

static inline void ring_internal_bounds_check(ssize idx, ssize length)
{
    ASSERT(idx < length);
}

static inline ssize ring_length_impl(ssize head, ssize tail, ssize capacity)
{
    ssize result = 0;
    if (head == - 1) {

    } else if (head < tail) {
        result = tail - head;
    } else {
        result = tail + capacity - head;
    }

    return result;
}

#endif //RING_BUFFER_H
