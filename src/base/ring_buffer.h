#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <string.h>

#include "base/utils.h"

/*
  TODO:
  - Return address of pushed value
  - ring_alloc - Doesn't push, just allocates space
  - If using comma operator to bounds check, can't take address of expression.
    Return address?
  - Clean up header
  - Better name for ring_pop_load
 */

/* Definition helpers */
#define DEFINE_HEAP_RING_BUFFER(type, name)	\
    typedef struct name {                       \
        type  *items;                           \
        ssize  head;                            \
        ssize  tail;                            \
        ssize  capacity;                        \
    } name

#define DEFINE_STATIC_RING_BUFFER(type, name, cap)      \
    typedef struct name {                               \
        type   items[cap];                              \
        ssize  head;                                    \
        ssize  tail;                                    \
        ssize  capacity;                                \
    } name

/* Initialization */
#define ring_initialize(buf, cap, allocator)                               \
    do {                                                                   \
        ASSERT(!multiply_overflows_ssize((cap), SIZEOF(*(buf)->items)));   \
        (buf)->items = allocate(allocator, (cap) * SIZEOF(*(buf)->items)); \
        (buf)->capacity = cap;						   \
	ring_impl_set_to_empty(&(buf)->head, &(buf)->tail);		   \
    } while (0)

#define ring_initialize_static(buf)					           \
    do {								           \
        (buf)->capacity = ARRAY_COUNT((buf)->items);			           \
        ASSERT(!multiply_overflows_ssize((buf)->capacity, SIZEOF(*(buf)->items))); \
        ring_impl_set_to_empty(&(buf)->head, &(buf)->tail);			   \
    } while (0)

/* Modifying operations */
#define ring_push(buf, item) ring_impl_push((buf)->items, &(buf)->head, \
        &(buf)->tail, (buf)->capacity, SIZEOF(*(buf)->items), (item))
#define ring_push_overwrite(buf, item) ring_impl_push_overwrite((buf)->items, \
        &(buf)->head, &(buf)->tail, (buf)->capacity, SIZEOF(*(buf)->items), (item))
#define ring_pop(buf) ring_impl_pop(&(buf)->head, &(buf)->tail, (buf)->capacity)
#define ring_pop_load(buf) ((buf)->items[ring_impl_pop_load(&(buf)->head, &(buf)->tail, (buf)->capacity)])
#define ring_pop_tail(buf) ring_impl_pop_tail(&(buf)->head, &(buf)->tail, (buf)->capacity)
#define ring_swap_remove(buf, index) (*ring_at(buf, index) = *ring_peek_tail(buf), ring_pop_tail(buf))

/* Access operations */
#define ring_at(buf, idx) (ring_impl_bounds_check((idx), ring_length((buf))), \
        &((buf)->items[((buf)->head + (idx)) % (buf)->capacity]))
#define ring_peek(buf) ring_at(buf, 0)
#define ring_peek_tail(buf) ring_at(buf, ring_length(buf) - 1)

/* Query operations */
#define ring_length(buf) ring_impl_length((buf)->head, (buf)->tail, (buf)->capacity)
#define ring_is_full(buf) ring_impl_is_full(&(buf)->head, &(buf)->tail)
#define ring_is_empty(buf) ring_impl_is_empty(&(buf)->head, &(buf)->tail)

/* Internal implementation functions */
static inline b32 ring_impl_is_empty(const ssize *head, const ssize *tail)
{
    b32 result = *head == -1;
    ASSERT(!result || (*tail == -1));

    return result;
}

static inline b32 ring_impl_is_full(const ssize *head, const ssize *tail)
{
    b32 result = !ring_impl_is_empty(head, tail) && (*head == *tail);
    return result;
}

static inline ssize ring_impl_length(ssize head, ssize tail, ssize capacity)
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

static inline void ring_impl_set_to_empty(ssize *head, ssize *tail)
{
    *head = -1;
    *tail = -1;
}

static inline ssize ring_impl_pop_load(ssize *head, ssize *tail, ssize capacity)
{
    ASSERT(!ring_impl_is_empty(head, tail));
    ssize previous_head = *head;

    ssize new_head = (previous_head + 1) % capacity;

    if (new_head == *tail) {
        ring_impl_set_to_empty(head, tail);
    } else {
        *head = new_head;
    }

    return previous_head;
}

static inline void ring_impl_pop(ssize *head, ssize *tail, ssize capacity)
{
    ring_impl_pop_load(head, tail, capacity);
}

static inline void ring_impl_pop_tail(ssize *head, ssize *tail, ssize capacity)
{
    ASSERT(!ring_impl_is_empty(head, tail));

    *tail -= 1;

    if (*tail == -1) {
        *tail = capacity - 1;
    }

    if (*tail == *head) {
        ring_impl_set_to_empty(head, tail);
    }
}

static inline void ring_impl_push(void *items, ssize *head, ssize *tail, ssize capacity,
    ssize item_size, void *item)
{
    ASSERT(items);
    ASSERT(capacity);
    ASSERT(!ring_impl_is_full(head, tail));

    if (*head == -1) {
        *head = 0;
        *tail = 0;
    }

    memcpy(byte_offset(items, *tail * item_size), item, cast_ssize_to_usize(item_size));
    *tail = (*tail + 1) % capacity;
}

static inline void ring_impl_push_overwrite(void *items, ssize *head, ssize *tail, ssize capacity,
    ssize item_size, void *item)
{
    ASSERT(items);
    ASSERT(capacity);

    // TODO: this can be made more efficient
    if (ring_impl_is_full(head, tail)) {
        ring_impl_pop(head, tail, capacity);
    }

    ring_impl_push(items, head, tail, capacity, item_size, item);
}

static inline void ring_impl_bounds_check(ssize idx, ssize length)
{
    ASSERT(idx < length);
}

#endif //RING_BUFFER_H
