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
#include "base/matrix.h"
#include "os/file.h"
#include "os/path.h"
#include "os/thread_context.h"
#include "image.h"

#include "render/backend/renderer_backend.h"

#include "os/window.h"
#include "shader.h"
#include "tests.c"

#define WINDOW_WIDTH 768
#define WINDOW_HEIGHT 468

int main()
{
    bool result = thread_ctx_initialize_system();
    ASSERT(result);
    thread_ctx_create_for_thread(default_allocator);

    LinearArena arena = arena_create(default_allocator, MB(4));
    Allocator alloc = arena_create_allocator(&arena);

    run_tests();

    return 0;

    /* ReadFileResult file = os_read_entire_file(str_literal("test.glsl"), alloc); */
    /* String src = { (char *)file.file_data, file.file_size }; */
    ShaderIncludeList list = shader_get_dependencies(str_literal("test.glsl"), alloc);

    for (ShaderIncludeDirective *dir = list.head; dir; dir = dir->next) {
        printf("%.*s\n", (s32)dir->absolute_include_path.length, dir->absolute_include_path.data);
    }

    printf("\n");

    for (ShaderIncludeDirective *dir = list.tail; dir; dir = dir->prev) {
        printf("%.*s\n", (s32)dir->absolute_include_path.length, dir->absolute_include_path.data);
    }




    Image img = img_load_png_from_file(str_literal("assets/sprites/test.png"), alloc);


    struct WindowHandle *handle = os_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "foo", WINDOW_FLAG_NON_RESIZABLE, alloc);
    RendererBackend *backend = renderer_backend_initialize(alloc);

    ReadFileResult read_result = os_read_entire_file(str_literal("assets/shaders/shader.glsl"), default_allocator);
    ASSERT(read_result.file_data);

    String source = { .data = (char *)read_result.file_data, .length = read_result.file_size };

    ShaderHandle *shader_handle = renderer_backend_create_shader(source, default_allocator);
    ASSERT(shader_handle);

    renderer_backend_use_shader(shader_handle);

    Matrix4 proj = mat4_orthographic(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT, 0.1f, 100.0f);
    proj = mat4_translate(proj, (Vector2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2});
    proj = mat4_scale(proj, 3.0f);

    renderer_backend_set_mat4_uniform(shader_handle, str_literal("u_proj"), proj);

    TextureHandle *texture = renderer_backend_create_texture(img, alloc);
    ASSERT(texture);

    renderer_backend_bind_texture(texture);

    while (!os_window_should_close(handle)) {
        renderer_backend_begin_frame(backend);

        Vertex a = {
            .position = {0, 0},
            .uv = {0, 0},
            .color = {1, 0, 0, 1}
        };

        Vertex b = {
            .position = {64, 0},
            .uv = {1, 0},
            .color = {0, 1, 0, 1}
        };

        Vertex c = {
            .position = {64, -64},
            .uv = {1, 1},
            .color = {0, 0, 1, 1}
        };

        Vertex d = {
            .position = {0, -64},
            .uv = {0, 1},
            .color = {1, 0, 1, 1}
        };

        renderer_backend_draw_quad(
            backend,
            a,
            b, c, d

        );

        renderer_backend_end_frame(backend);
        os_poll_events(handle);
    }

    os_destroy_window(handle);

    arena_destroy(&arena);

    return 0;
}
