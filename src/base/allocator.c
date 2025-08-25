#include "allocator.h"

void *DefaultAllocate(void *ctx, s64 item_count, s64 item_size, s64 alignment)
{
    (void)ctx;
    (void)alignment;

    if (MultiplicationOverflows_ssize(item_count, item_size)) {
        return 0;
    }

    const s64 byte_count = item_count * item_size;

    void *ptr = malloc(Cast_s64_usize(byte_count));
    Assert(IsAligned((s64)ptr, alignment));

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
