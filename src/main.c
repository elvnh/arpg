#include <GLFW/glfw3.h>
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
#include "base/matrix.h"
#include "os/file.h"
#include "os/path.h"
#include "os/thread_context.h"

#include "render/backend/renderer_backend.h"

#include "os/window.h"
#include "tests.c"

#define WINDOW_WIDTH 768
#define WINDOW_HEIGHT 468

int main()
{
    bool result = thread_ctx_initialize_system();
    ASSERT(result);
    thread_ctx_create_for_thread(default_allocator);

    run_tests();

    LinearArena arena = arena_create(default_allocator, MB(4));
    Allocator alloc = arena_create_allocator(&arena);

    struct WindowHandle *handle = os_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "foo", WINDOW_FLAG_NON_RESIZABLE, alloc);
    RendererBackend *backend = renderer_backend_initialize(alloc);

    ReadFileResult read_result = os_read_entire_file(str_literal("assets/shaders/shader.glsl"), default_allocator);
    ASSERT(read_result.file_data);

    String source = { .data = (char *)read_result.file_data, .length = read_result.file_size };

    ShaderHandle *shader_handle = renderer_backend_compile_shader(source, default_allocator);
    ASSERT(shader_handle);

    renderer_backend_use_shader(shader_handle);

    Matrix4 proj = mat4_orthographic(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT, 0.1f, 100.0f);
    renderer_backend_set_mat4_uniform(shader_handle, str_literal("u_proj"), proj);

    while (!os_window_should_close(handle)) {
        renderer_backend_begin_frame(backend);

        Vertex a = {
            .position = {0, 0},
            .uv = {0},
            .color = {1, 0, 0, 1}
        };

        Vertex b = {
            .position = {50, 100},
            .uv = {0},
            .color = {0, 1, 0, 1}
        };

        Vertex c = {
            .position = {100, 0},
            .uv = {0},
            .color = {0, 0, 1, 1}
        };

        renderer_backend_draw_triangle(
            backend,
            a,
            b, c

        );

        renderer_backend_end_frame(backend);
        os_poll_events(handle);
    }

    os_destroy_window(handle);

    arena_destroy(&arena);

    return 0;
}
