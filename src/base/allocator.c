#include "allocator.h"

void *DefaultAllocate(void *ctx, ssize item_count, ssize item_size, ssize alignment)
{
    (void)ctx;
    (void)alignment;

    if (MultiplicationOverflows_ssize(item_count, item_size)) {
        return 0;
    }

    const ssize byte_count = item_count * item_size;

    void *ptr = malloc(Cast_s64_usize(byte_count));
    Assert(IsAligned((ssize)ptr, alignment));

    return ptr;
}

void DefaultFree(void *ctx, void *ptr)
{
    (void)ctx;
    free(ptr);
}

bool DefaultTryExtend(void *ctx, void *ptr, ssize old_size, ssize new_size)
{
    (void)ctx;
    (void)ptr;
    (void)old_size;
    (void)new_size;

    return false;
}
