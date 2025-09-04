#include "allocator.h"
#include "base/utils.h"

void *default_allocate(void *ctx, ssize item_count, ssize item_size, ssize alignment)
{
    (void)ctx;

    void *ptr = calloc((size_t)item_count, (size_t)item_size);
    ASSERT(is_aligned((ssize)ptr, alignment));

    return ptr;
}

void default_deallocate(void *ctx, void *ptr)
{
    (void)ctx;
    free(ptr);
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
