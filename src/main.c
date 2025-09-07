#include <dlfcn.h>

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#include "base/free_list_arena.h"
#include "base/string8.h"
#include "platform/input.h"
#include "platform/path.h"
#include "renderer/renderer_backend.h"
#include "game/assets.h"

#include "platform/window.h"
#include "base/thread_context.h"
#include "game/renderer/render_command.h"
#include "base/vertex.h"
#include "tests.c"

#define WINDOW_WIDTH 768
#define WINDOW_HEIGHT 468

#include "game/renderer/render_batch.h"

typedef struct {
    ShaderHandle shader;
    TextureHandle texture;
} RendererState;

#if 0
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
#endif

#include "game/game.h"

typedef void (*GameUpdateAndRender)(GameState *, RenderBatch *);

typedef struct {
    void *handle;
    GameUpdateAndRender update_and_render;
} GameFunctions;

static void load_game_functions(GameFunctions *functions)
{
    LinearArena scratch = la_create(default_allocator, 1024);
    String path = os_get_executable_directory(la_allocator(&scratch), &scratch);
    path = str_concat(path, str_lit("/libgame.so"), la_allocator(&scratch));
    path = str_null_terminate(path, la_allocator(&scratch));

    if (functions->handle) {
        functions->update_and_render = 0;
        dlclose(functions->handle);
    }

    functions->update_and_render = 0;
    functions->handle = 0;

    void *handle = dlopen(path.data, RTLD_NOW);

    if (handle) {
        void *func = dlsym(handle, "game_update_and_render");
        ASSERT(handle);
        ASSERT(func);

        functions->handle = handle;
        functions->update_and_render = (GameUpdateAndRender)func;

    }
    la_destroy(&scratch);
}

int main()
{
    /* bool result = thread_ctx_initialize_system(); */
    /* ASSERT(result); */
    /* thread_ctx_create_for_thread(default_allocator); */

    LinearArena arena = la_create(default_allocator, MB(4));
    Allocator alloc = la_allocator(&arena);

    run_tests();

    WindowHandle *window = os_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "foo", WINDOW_FLAG_NON_RESIZABLE, alloc);
    RendererBackend *backend = renderer_backend_initialize(alloc);

    Matrix4 proj = mat4_orthographic(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT, 0.1f, 100.0f);
    proj = mat4_translate(proj, (Vector2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2});
    proj = mat4_scale(proj, 3.0f);

    //AssetSystem assets = assets_initialize(alloc);

    LinearArena scratch = la_create(default_allocator, MB(4));

#if 0
    ShaderHandle shader_handle = assets_register_shader(&assets, str_lit("assets/shaders/shader.glsl"), scratch);
    TextureHandle texture_handle = assets_register_texture(&assets, str_lit("assets/sprites/test.png"), scratch);

    {
        ShaderAsset *shader_asset = assets_get_shader(&assets, shader_handle);
        renderer_backend_use_shader(shader_asset);
        renderer_backend_set_uniform_mat4(shader_asset, str_lit("u_proj"), proj);
    }
#endif

    RenderBatch rb = {0};

    GameState game_state = {
        .frame_arena = la_create(default_allocator, MB(4)),
        .shader = (ShaderHandle){0},
        .texture = (TextureHandle){0},
        /* .shader = shader_handle, */
        /* .texture = texture_handle */
    };

    GameFunctions game_code = {0};
    load_game_functions(&game_code);

    Input input = {0};

    while (!os_window_should_close(window)) {
        input_update(&input, window);

        load_game_functions(&game_code);

        renderer_backend_begin_frame(backend);

        rb.entry_count = 0;

        if (game_code.handle) {
            game_code.update_and_render(&game_state, &rb);
        }
        //execute_render_commands(&rb, &assets, backend);

        os_poll_events(window);
    }

    os_destroy_window(window);
    la_destroy(&arena);

    return 0;
}
