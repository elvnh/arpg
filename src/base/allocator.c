#include "allocator.h"

void *default_allocate(void *ctx, ssize item_count, ssize item_size, ssize alignment)
{
    (void)ctx;
    (void)alignment;

    if (multiply_overflows_ssize(item_count, item_size)) {
        return 0;
    }

    const ssize byte_count = item_count * item_size;

    void *ptr = malloc(cast_s64_to_usize(byte_count));
    ASSERT(is_aligned((ssize)ptr, alignment));

    return ptr;
}

void default_deallocate(void *ctx, void *ptr)
{
    (void)ctx;
    free(ptr);
}

bool default_try_resize(void *ctx, void *ptr, ssize old_size, ssize new_size)
{
    (void)ctx;
    (void)ptr;
    (void)old_size;
    (void)new_size;

    return false;
}

void *stub_allocate(void *ctx, ssize item_count, ssize item_size, ssize alignment)
{
    (void)ctx;
    (void)item_count;
    (void)item_size;
    (void)alignment;

    return 0;
}

void stub_deallocate(void *ctx, void *ptr)
{
    (void)ctx;
    (void)ptr;
}

bool stub_try_resize(void *ctx, void *ptr, ssize old_size, ssize new_size)
{
    (void)ctx;
    (void)ptr;
    (void)old_size;
    (void)new_size;

    return false;
}
