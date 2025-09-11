#include <dlfcn.h>

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/string8.h"
#include "base/image.h"
#include "base/list.h"
#include "game/renderer/render_key.h"
#include "input.h"
#include "platform.h"
#include "renderer/renderer_backend.h"
#include "asset_manager.h"

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

static RendererState switch_renderer_state(RendererState new_state, AssetManager *assets,
    RendererState old_state, RendererBackend *backend)
{
    (void)backend;

    if (new_state.texture.id != old_state.texture.id) {
        if (new_state.texture.id != NULL_TEXTURE.id) {
            TextureAsset *texture = assets_get_texture(assets, new_state.texture);
            renderer_backend_bind_texture(texture);
        }
    }

    if (new_state.shader.id != old_state.shader.id) {
        ShaderAsset *shader = assets_get_shader(assets, new_state.shader);

        renderer_backend_use_shader(shader);
    }

    return new_state;
}

#define INVALID_RENDERER_STATE (RendererState){{(AssetID)-1}, {(AssetID)-1}}

static void execute_render_commands(RenderBatch *rb, AssetManager *assets,
    RendererBackend *backend, LinearArena *scratch)
{
    if (rb->entry_count == 0) {
        return;
    }

    RendererState current_state = get_state_needed_for_entry(rb->entries[0].key);
    switch_renderer_state(current_state, assets, INVALID_RENDERER_STATE, backend);

    for (ssize i = 0; i < rb->entry_count; ++i) {
        RenderEntry *entry = &rb->entries[i];
        RenderCmdHeader *header = entry->data;

        RendererState needed_state = get_state_needed_for_entry(entry->key);

        if (renderer_state_change_needed(current_state, needed_state)) {
            renderer_backend_flush(backend);
            current_state = switch_renderer_state(needed_state, assets, current_state, backend);
        }

        for (SetupCmdHeader *setup_cmd = header->first_setup_command; setup_cmd; setup_cmd = setup_cmd->next) {
            switch (setup_cmd->kind) {
                case SETUP_CMD_SET_UNIFORM_VEC4: {
                    SetupCmdUniformVec4 *cmd = (SetupCmdUniformVec4 *)setup_cmd;
                    ShaderAsset *shader = assets_get_shader(assets, current_state.shader);

                    renderer_backend_set_uniform_vec4(shader, cmd->uniform_name, cmd->vector, scratch);
                } break;

                INVALID_DEFAULT_CASE;
            }
        }

        switch (entry->data->kind) {
            case RENDER_CMD_RECTANGLE: {
                RectangleCmd *cmd = (RectangleCmd *)entry->data;
                RectangleVertices verts = rect_get_vertices(cmd->rect, cmd->color);

                renderer_backend_draw_quad(backend, verts.top_left, verts.top_right,
		    verts.bottom_right, verts.bottom_left);
            } break;

            case RENDER_CMD_CIRCLE: {
                CircleCmd *cmd = (CircleCmd *)entry->data;

                s32 segments = 64;
                f32 radius = cmd->radius;
                f32 posx = cmd->position.x;
                f32 posy = cmd->position.y;
                RGBA32 color = cmd->color;

                Vertex a = { {posx, posy}, {0.5f, 0.5f}, color };

                f32 step_angle = (2.0f * PI) / (f32)segments;

                for (s32 k = 0; k < segments; ++k) {
                    f32 segment = (f32)k;

                    f32 x1 = posx + radius * cosf(step_angle * segment);
                    f32 y1 = posy + radius * sinf(step_angle * segment);
                    f32 tx1 = (x1 / radius + 1.0f) * 0.5f;
                    f32 ty1 = 1.0f - ((y1 / radius + 1.0f) * 0.5f);

                    f32 x2 = posx + radius * cosf(step_angle * (segment + 1));
                    f32 y2 = posy + radius * sinf(step_angle * (segment + 1));
                    f32 tx2 = (x2 / radius + 1.0f) * 0.5f;
                    f32 ty2 = 1.0f - ((y2 / radius + 1.0f) * 0.5f);


                    Vertex b = { {x1, y1}, {tx1, ty1}, color};
                    Vertex c = { {x2, y2}, {tx2, ty2}, color};

                    renderer_backend_draw_triangle(backend, a, b, c);
                }


            } break;

           INVALID_DEFAULT_CASE;
        }
    }

    renderer_backend_flush(backend);
}

static void sort_render_commands(RenderBatch *rb)
{
    (void)rb;
}

#include "game/game.h"

typedef void (GameUpdateAndRender)(GameState *, RenderBatch *, const AssetList *, FrameData);

typedef struct {
    void *handle;
    GameUpdateAndRender *update_and_render;
} GameCode;

#define GAME_SO_NAME "libgame.so"

static void load_game_code(GameCode *game_code, String so_path, LinearArena *scratch)
{
    so_path = str_null_terminate(so_path, la_allocator(scratch));
    String bin_dir = platform_get_parent_path(so_path, la_allocator(scratch), scratch);

    {
        String lock_file_path = str_concat(bin_dir, str_lit("/build/lock"), la_allocator(scratch));
        while (platform_file_exists(lock_file_path, scratch));
    }

    void *handle = dlopen(so_path.data, RTLD_NOW);
    ASSERT(handle);

    void *func = dlsym(handle, "game_update_and_render");
    ASSERT(func);

    game_code->handle = handle;
    game_code->update_and_render = (GameUpdateAndRender *)func;
}

static void unload_game_code(GameCode *game_code)
{
    game_code->update_and_render = 0;
    dlclose(game_code->handle);
    game_code->handle = 0;
}

#include <sys/inotify.h>
#include <sys/poll.h>
#include <unistd.h>

typedef struct {
    Allocator allocator;
    pthread_t thread;
    Mutex lock;
    StringList asset_reload_queue;
    b32 should_terminate;
} AssetWatcherContext;

#define INOTIFY_MAX_BUFFER_LENGTH (sizeof(struct inotify_event) + NAME_MAX + 1)

static String get_assets_directory(LinearArena *arena)
{
    String executable_dir = platform_get_executable_directory(la_allocator(arena), arena);
    String result = str_concat(executable_dir, str_lit("/"ASSET_DIRECTORY), la_allocator(arena));
    result = str_null_terminate(result, la_allocator(arena));

    return result;
}

static String get_shader_directory(LinearArena *arena)
{
    String executable_dir = platform_get_executable_directory(la_allocator(arena), arena);
    String result = str_concat(executable_dir, str_lit("/"SHADER_DIRECTORY), la_allocator(arena));
    result = str_null_terminate(result, la_allocator(arena));

    return result;
}

static String get_sprite_directory(LinearArena *arena)
{
    String executable_dir = platform_get_executable_directory(la_allocator(arena), arena);
    String result = str_concat(executable_dir, str_lit("/"SPRITE_DIRECTORY), la_allocator(arena));
    result = str_null_terminate(result, la_allocator(arena));

    return result;
}

void *file_watcher_thread(void *user_data)
{
    AssetWatcherContext *ctx = user_data;
    LinearArena scratch = la_create(ctx->allocator, MB(1)) ;

    s32 fd = inotify_init();
    ASSERT(fd >= 0);

    char event_buffer[INOTIFY_MAX_BUFFER_LENGTH];

    String root_path = get_assets_directory(&scratch);
    String shader_path = get_shader_directory(&scratch);
    String sprite_path = get_sprite_directory(&scratch);

    s32 root_wd = inotify_add_watch(fd, root_path.data, IN_MODIFY);
    s32 shader_wd = inotify_add_watch(fd, shader_path.data, IN_MODIFY);
    s32 sprite_wd = inotify_add_watch(fd, sprite_path.data, IN_MODIFY);
    ASSERT(root_wd != -1);
    ASSERT(shader_wd != -1);
    ASSERT(sprite_wd != -1);

    struct pollfd poll_desc = {
        .fd = fd,
        .events = POLLIN,
        .revents = 0
    };

    s32 timeout = 100;

    while (!atomic_load_s32(&ctx->should_terminate)) {
        s32 poll_result = poll(&poll_desc, 1, timeout);
        ASSERT(poll_result >= 0);

        if (poll_result == 0) {
            continue;
        }

        ssize length = read(fd, event_buffer, INOTIFY_MAX_BUFFER_LENGTH);
        ssize buffer_offset = 0;

        while (buffer_offset < length) {
            struct inotify_event *event = (struct inotify_event *)&event_buffer[buffer_offset];

            ASSERT(event->mask & IN_MODIFY);

            if (event->mask & IN_MODIFY) {
                String parent_path = {0};

                if (event->wd == shader_wd) {
                    parent_path = shader_path;
                } else if (event->wd == sprite_wd) {
                    parent_path = sprite_path;
                } else {
                    ASSERT(0);
                }

                String name = { event->name, (ssize)strlen(event->name) };
                StringNode *modified_file = allocate_item(ctx->allocator, StringNode);

                modified_file->data = str_concat(parent_path, name, ctx->allocator);

		mutex_lock(ctx->lock);
                list_push_back(&ctx->asset_reload_queue, modified_file);
		mutex_release(ctx->lock);
            }

            buffer_offset += (SIZEOF(struct inotify_event) + event->len);
        }
    }

    la_destroy(&scratch);

    return 0;
}

static void file_watcher_start(AssetWatcherContext *ctx)
{
    ctx->allocator = default_allocator;

    ctx->lock = mutex_create(ctx->allocator);
    pthread_create(&ctx->thread, 0, file_watcher_thread, ctx); // move to init
}

static void file_watcher_stop(AssetWatcherContext *ctx)
{
    atomic_store_s32(&ctx->should_terminate, true);
    pthread_join(ctx->thread, 0);
    mutex_destroy(ctx->lock, ctx->allocator);
}

int main()
{
    LinearArena arena = la_create(default_allocator, MB(4));
    Allocator alloc = la_allocator(&arena);

    run_tests();

    WindowHandle *window = platform_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "foo",
	WINDOW_FLAG_NON_RESIZABLE, alloc);
    RendererBackend *backend = renderer_backend_initialize(alloc);

    AssetManager assets = assets_initialize(alloc);
    LinearArena scratch = la_create(default_allocator, MB(4));

    AssetList asset_list = {
        .shader = assets_register_shader(&assets, str_lit("shader.glsl"), &scratch),
        .shader2 = assets_register_shader(&assets, str_lit("shader2.glsl"), &scratch),
        .texture = assets_register_texture(&assets, str_lit("test.png"), &scratch),
        .white_texture = assets_register_texture(&assets, str_lit("white.png"), &scratch),
    };

    RenderBatch rb = {0};

    GameState game_state = {
        .frame_arena = la_create(default_allocator, MB(4))
    };

    String executable_dir = platform_get_executable_directory(alloc, &scratch);
    String so_path = str_concat(executable_dir, str_lit("/"GAME_SO_NAME), alloc);

    GameCode game_code = {0};
    load_game_code(&game_code, so_path, &scratch);

    Input input = {0};

    Timestamp game_so_load_time = platform_get_time();

    AssetWatcherContext asset_watcher = {0};
    file_watcher_start(&asset_watcher);

    while (!platform_window_should_close(window)) {
        {
            // TODO: hot reload game code this way too
            mutex_lock(asset_watcher.lock);

            for (StringNode *file = list_head(&asset_watcher.asset_reload_queue); file;) {
                StringNode *next = file->next;
                AssetSlot *slot = assets_get_asset_by_path(&assets, file->data, &scratch);

                b32 should_pop = true;

                if (slot) {
                    b32 reloaded = assets_reload_asset(&assets, slot, &scratch);

                    if (reloaded) {
                        printf("Reloaded asset '%s'.\n",
                            str_null_terminate(file->data, la_allocator(&scratch)).data);
                    } else {
                        // Failed to reload, try again later
                        should_pop = false;
                    }
                }

                if (should_pop) {
                    list_pop_head(&asset_watcher.asset_reload_queue);
                    deallocate(asset_watcher.allocator, file->data.data);
                    deallocate(asset_watcher.allocator, file);
                }

                file = next;
            }

            mutex_release(asset_watcher.lock);
        }

        {
            ShaderAsset *shader_asset = assets_get_shader(&assets, asset_list.shader);
            renderer_backend_use_shader(shader_asset);

            Matrix4 proj = mat4_orthographic(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT, 0.1f, 100.0f);
            proj = mat4_translate(proj, (Vector2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2});
            proj = mat4_scale(proj, 3.0f);
            renderer_backend_set_global_projection(backend, proj);
        }

        Timestamp so_mod_time = platform_get_file_info(so_path, &scratch).last_modification_time;

        if (timestamp_less_than(game_so_load_time, so_mod_time)) {
            unload_game_code(&game_code);
            load_game_code(&game_code, so_path, &scratch);

            game_so_load_time = platform_get_time();
            printf("Hot reloaded game.\n");
        }

        platform_update_input(&input, window);

        renderer_backend_clear(backend);
        rb.entry_count = 0;

        // TODO: proper dt
        f32 dt = 0.16f;

        FrameData frame_data = {
            .dt = dt,
            .input = &input
        };

        game_code.update_and_render(&game_state, &rb, &asset_list, frame_data);

        execute_render_commands(&rb, &assets, backend, &scratch);

        platform_poll_events(window);
    }

    platform_destroy_window(window);
    la_destroy(&arena);

    file_watcher_stop(&asset_watcher);

    return 0;
}
