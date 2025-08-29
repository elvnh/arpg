#ifndef THREAD_CONTEXT_H
#define THREAD_CONTEXT_H

#include "base/allocator.h"
#include "base/linear_arena.h"

typedef struct {
    u64          thread_id;
    LinearArena  scratch_arena;
} thread_context;

bool             thread_ctx_initialize_system();
bool             thread_ctx_create_for_thread(Allocator allocator);
thread_context  *thread_ctx_get();
LinearArena     *thread_ctx_get_arena();
Allocator        thread_ctx_get_allocator();

#endif //THREAD_CONTEXT_H
