#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "base/allocator.h"

#define da_init(arr, cap, alloc)                                        \
    do {                                                                \
        (arr)->items = allocate_aligned((alloc), cap,                   \
            SIZEOF((*(arr)->items)), ALIGNOF((*(arr)->items)));         \
        (arr)->capacity = (cap);                                        \
        (arr)->count = 0;                                               \
    } while (0)

#define da_push(arr, item, alloc)                                       \
    do {                                                                \
        (arr)->items = impl_da_maybe_grow((alloc), (arr)->items,        \
            (arr)->count, &((arr)->capacity),                           \
            SIZEOF((*(arr)->items)), ALIGNOF(*((arr)->items)));         \
        ASSERT((arr)->items);                                           \
        (arr)->items[(arr)->count++] = (item);                          \
    } while (0)

static inline void *impl_da_maybe_grow(Allocator alloc, void *items,
    ssize count, ssize *capacity, ssize item_size, ssize alignment)
{
    ssize old_cap = *capacity;
    ASSERT(count <= old_cap);

    void *result = items;

    if (count == old_cap) {
        ssize new_cap = (old_cap == 0)
            ? 16
            : old_cap * 2;

        result = allocate_aligned(alloc, new_cap, item_size, alignment);
        // TODO: implement realloc for allocators
        if (items) {
            memcpy(result, items, (usize)(old_cap * item_size));
        }

        deallocate(alloc, items);

        *capacity = new_cap;
    }

    return result;
}

#endif //DYNAMIC_ARRAY_H
