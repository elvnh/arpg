#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#include "base/typedefs.h"
#include "base/string8.h"
#include "base/linear_arena.h"
#include "base/allocator.h"
#include "base/utils.h"
#include "base/linked_list.h"
#include "os/file.h"
#include "os/path.h"
#include "os/thread_context.h"

#include "render/backend/renderer_backend.h"

#include "os/window.h"
#include "tests.c"

int main()
{
    bool result = thread_ctx_initialize_system();
    ASSERT(result);
    thread_ctx_create_for_thread(default_allocator);

    run_tests();

    LinearArena arena = arena_create(default_allocator, MB(4));
    Allocator alloc = arena_create_allocator(&arena);

    struct WindowHandle *handle = os_create_window(256, 256, "foo", 0, alloc);

    RendererBackend *backend = renderer_backend_initialize(alloc);

    ReadFileResult read_result = os_read_entire_file(str_literal("assets/shaders/shader.glsl"), default_allocator);
    ASSERT(read_result.file_data);

    String source = { .data = (char *)read_result.file_data, .length = read_result.file_size };

    ShaderHandle *shader_handle = renderer_backend_compile_shader(source, default_allocator);
    ASSERT(shader_handle);

    os_destroy_window(handle);

    arena_destroy(&arena);

    return 0;
}
