#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#include "base/free_list_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
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
#include "assets.h"

#include "os/window.h"
#include "render/render_command.h"
#include "render/vertex.h"
#include "shader.h"
#include "tests.c"

#define WINDOW_WIDTH 768
#define WINDOW_HEIGHT 468

#include "render/render_batch.h"

typedef struct {
    ShaderHandle shader;
    TextureHandle texture;
} RendererState;

static RendererState get_state_needed_for_entry(RenderKey key)
{
    RenderKey shader_id = render_key_extract_shader(key);
    RenderKey texture_id = render_key_extract_texture(key);

    RendererState result = {
        .shader = (ShaderHandle){(u32)shader_id},
        .texture = (TextureHandle){(u32)texture_id}
    };

    return result;
}

static b32 renderer_state_change_needed(RendererState lhs, RendererState rhs)
{
    return (lhs.shader.id != rhs.shader.id) || (lhs.texture.id != rhs.texture.id);
}

static RendererState switch_renderer_state(RendererState state, AssetSystem *assets, RendererBackend *backend)
{
    {
        TextureAsset *texture = assets_get_texture(assets, state.texture);
        renderer_backend_bind_texture(texture);
    }

    {
        ShaderAsset *shader = assets_get_shader(assets, state.shader);
        renderer_backend_use_shader(shader);
    }

    return state;
}

static void execute_render_commands(RenderBatch *rb, AssetSystem *assets, RendererBackend *backend)
{
    RendererState state = {0};

    for (ssize i = 0; i < rb->entry_count; ++i) {
        RenderEntry *entry = &rb->entries[i];

        RendererState needed_state = get_state_needed_for_entry(entry->key);

        if (renderer_state_change_needed(state, needed_state) || (i == 0)) {
            state = switch_renderer_state(needed_state, assets, backend);
            renderer_backend_end_frame(backend);
        }

        for (SetupCmdHeader *setup_cmd = entry->data->first_setup_command; setup_cmd; setup_cmd = setup_cmd->next) {
            switch (setup_cmd->kind) {
                case SETUP_CMD_SET_UNIFORM_VEC4: {
                    SetupCmdUniformVec4 *cmd = (SetupCmdUniformVec4 *)setup_cmd;
                    ShaderAsset *shader = assets_get_shader(assets, state.shader);

                    renderer_backend_set_uniform_vec4(shader, cmd->uniform_name, cmd->vector);
                } break;
                INVALID_DEFAULT_CASE;
            }
        }

        switch (entry->data->kind) {
            case RENDER_CMD_SPRITE: {
                SpriteCmd *cmd = (SpriteCmd *)entry->data;
                RectangleVertices verts = rect_get_vertices(cmd->rect, RGBA32_WHITE);

                renderer_backend_draw_quad(backend, verts.top_left, verts.top_right, verts.bottom_right, verts.bottom_left);
            } break;

           INVALID_DEFAULT_CASE;
        }
    }

    renderer_backend_end_frame(backend);
}

static void sort_render_commands(RenderBatch *rb)
{
    (void)rb;
}

int main()
{
    bool result = thread_ctx_initialize_system();
    ASSERT(result);
    thread_ctx_create_for_thread(default_allocator);

    LinearArena arena = arena_create(default_allocator, MB(4));
    Allocator alloc = arena_create_allocator(&arena);

    run_tests();

    WindowHandle *handle = os_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "foo", WINDOW_FLAG_NON_RESIZABLE, alloc);
    RendererBackend *backend = renderer_backend_initialize(alloc);

    Matrix4 proj = mat4_orthographic(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT, 0.1f, 100.0f);
    proj = mat4_translate(proj, (Vector2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2});
    proj = mat4_scale(proj, 3.0f);

    AssetSystem assets = assets_initialize(alloc);

    ShaderHandle shader_handle = assets_register_shader(&assets, str_lit("assets/shaders/shader.glsl"));
    TextureHandle texture_handle = assets_register_texture(&assets, str_lit("assets/sprites/test.png"));

    {
        ShaderAsset *shader_asset = assets_get_shader(&assets, shader_handle);
        renderer_backend_use_shader(shader_asset);
        renderer_backend_set_uniform_mat4(shader_asset, str_lit("u_proj"), proj);
    }

    RenderBatch rb = {0};

    while (!os_window_should_close(handle)) {
        renderer_backend_begin_frame(backend);

        rb.entry_count = 0;

        Rectangle rect = {
            .position = {0},
            .size = {64, -64}
        };

        rb_push_sprite(&rb, &arena, texture_handle, rect, shader_handle, 0);

        execute_render_commands(&rb, &assets, backend);

        os_poll_events(handle);

    }
    os_destroy_window(handle);

    arena_destroy(&arena);

    return 0;
}
