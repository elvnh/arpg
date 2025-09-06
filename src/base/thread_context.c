#include <pthread.h>

#include "base/linear_arena.h"
#include "base/thread_context.h"

#define THREAD_SCRATCH_ARENA_SIZE MB(4)

static pthread_key_t global_tld_key;

bool thread_ctx_initialize_system()
{
    s32 create_result = pthread_key_create(&global_tld_key, 0);

    if (create_result != 0) {
        return false;
    }

    return true;
}

bool thread_ctx_create_for_thread(Allocator allocator)
{
    thread_context *ctx = allocate_item(allocator, thread_context);
    ctx->thread_id = pthread_self();
    ctx->scratch_arena = la_create(allocator, THREAD_SCRATCH_ARENA_SIZE);

    s32 set_result = pthread_setspecific(global_tld_key, ctx);

    if (set_result != 0) {
        return false;
    }

    return true;
}

thread_context *thread_ctx_get()
{
    thread_context *ctx = pthread_getspecific(global_tld_key);
    ASSERT(ctx);

    return ctx;
}

LinearArena *thread_ctx_get_arena()
{
    thread_context *ctx = thread_ctx_get();

    return &ctx->scratch_arena;
}

LinearArena thread_ctx_get_temp_arena()
{
    return *thread_ctx_get_arena();
}

Allocator thread_ctx_get_allocator()
{
    LinearArena *arena = thread_ctx_get_arena();
    Allocator allocator = la_allocator(arena);

    return allocator;
}
