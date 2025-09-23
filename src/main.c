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

// TODO: move elsewhere
static void render_line(RendererBackend *backend, Vector2 start, Vector2 end, f32 thickness, RGBA32 color)
{
    Vector2 dir_r = v2_mul_s(v2_norm(v2_sub(end, start)), thickness / 2.0f);
    Vector2 dir_l = v2_neg(dir_r);
    Vector2 dir_u = { -dir_r.y, dir_r.x };
    Vector2 dir_d = v2_neg(dir_u);

    Vector2 tl = v2_add(v2_add(start, dir_l), dir_u);
    Vector2 tr = v2_add(end, v2_add(dir_r, dir_u));
    Vector2 br = v2_add(end, v2_add(dir_d, dir_r));
    Vector2 bl = v2_add(start, v2_add(dir_d, dir_l));

    // TODO: UV constants to make this easier to remember
    Vertex vtl = {
	.position = tl,
	.uv = {0,1},
	.color = color
    };

    Vertex vtr = {
	.position = tr,
	.uv = {1, 1},
	.color = color
    };

    Vertex vbr = {
	.position = br,
	.uv = {1, 0},
	.color = color
    };

    Vertex vbl = {
	.position = bl,
	.uv = {0, 0},
	.color = color
    };

    renderer_backend_draw_quad(backend, vtl, vtr, vbr, vbl);
}

static void execute_render_commands(RenderBatch *rb, AssetManager *assets,
    RendererBackend *backend, LinearArena *scratch)
{
    if (rb->entry_count == 0) {
        return;
    }

    RendererState current_state = get_state_needed_for_entry(rb->entries[0].key);
    switch_renderer_state(current_state, assets, INVALID_RENDERER_STATE, backend);

    renderer_backend_set_global_projection(backend, rb->projection);

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

            case RENDER_CMD_OUTLINED_RECTANGLE: {
                OutlinedRectangleCmd *cmd = (OutlinedRectangleCmd *)entry->data;

		RGBA32 color = cmd->color;
		f32 thick = cmd->thickness;

		Vector2 tl = rect_top_left(cmd->rect);
		Vector2 tr = rect_top_right(cmd->rect);
		Vector2 br = rect_bottom_right(cmd->rect);
		Vector2 bl = rect_bottom_left(cmd->rect);

		// TODO: These lines overlap which looks wrong when color is transparent
		render_line(backend, tl, v2_sub(tr, v2(thick, 0.0f)), thick, color);
		render_line(backend, tr, v2_add(br, v2(0.0f, thick)), thick, color);
		render_line(backend, br, v2_add(bl, v2(thick, 0.0f)), thick, color);
		render_line(backend, bl, v2_sub(tl, v2(0.0f, thick)), thick, color);

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

            case RENDER_CMD_LINE: {
                LineCmd *cmd = (LineCmd *)entry->data;

		render_line(backend, cmd->start, cmd->end, cmd->thickness, cmd->color);
            } break;

           INVALID_DEFAULT_CASE;
        }
    }

    renderer_backend_flush(backend);
}

#include "game/game.h"

typedef void (GameInitialize)(GameState *, GameMemory *);
typedef void (GameUpdateAndRender)(GameState *, RenderBatchList *, const AssetList *, FrameData, GameMemory *);

typedef struct {
    void *handle;
    GameInitialize *initialize;
    GameUpdateAndRender *update_and_render;
} GameCode;

#define GAME_SO_NAME "libgame.so"
#define BEGIN_IGNORE_FUNCTION_PTR_WARNINGS              \
    _Pragma("GCC diagnostic push");                     \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"");
#define END_IGNORE_FUNCTION_PTR_WARNINGS _Pragma("GCC diagnostic pop")

static void load_game_code(GameCode *game_code, String so_path, LinearArena *scratch)
{
    so_path = str_null_terminate(so_path, la_allocator(scratch));
    String bin_dir = platform_get_parent_path(so_path, la_allocator(scratch), scratch);

    {
        String lock_file_path = str_concat(bin_dir, str_lit("/build/lock"), la_allocator(scratch));
        while (platform_file_exists(lock_file_path, scratch));
    }

    void *handle = dlopen(so_path.data, RTLD_NOW);

    if (!handle) {
        goto error;
    }

    void *initialize = dlsym(handle, "game_initialize");
    void *update_and_render = dlsym(handle, "game_update_and_render");

    if (!initialize || !update_and_render) {
        goto error;
    }

    ASSERT(initialize);
    ASSERT(update_and_render);

    BEGIN_IGNORE_FUNCTION_PTR_WARNINGS;

    game_code->handle = handle;
    game_code->initialize = initialize;
    game_code->update_and_render = (GameUpdateAndRender *)update_and_render;

    END_IGNORE_FUNCTION_PTR_WARNINGS;

    return;

  error:
    fprintf(stderr, "%s\n", dlerror());
    ASSERT(0);
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

    ALIGNAS_T(struct inotify_event) char event_buffer[INOTIFY_MAX_BUFFER_LENGTH];

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

static void file_watcher_reload_modified_assets(AssetWatcherContext *ctx, AssetManager *asset_mgr,
    LinearArena *scratch)
{
    mutex_lock(ctx->lock);

    for (StringNode *file = list_head(&ctx->asset_reload_queue); file;) {
        StringNode *next = file->next;
        AssetSlot *slot = assets_get_asset_by_path(asset_mgr, file->data, scratch);

        b32 should_pop = true;

        if (slot) {
            b32 reloaded = assets_reload_asset(asset_mgr, slot, scratch);

            if (reloaded) {
                printf("Reloaded asset '%s'.\n",
                    str_null_terminate(file->data, la_allocator(scratch)).data);
            } else {
                // Failed to reload, try again later
                should_pop = false;
            }
        }

        if (should_pop) {
            list_pop_head(&ctx->asset_reload_queue);
            deallocate(ctx->allocator, file->data.data);
            deallocate(ctx->allocator, file);
        }

        file = next;
    }

    mutex_release(ctx->lock);
}

#define GAME_MEMORY_SIZE MB(32)
#define PERMANENT_ARENA_SIZE GAME_MEMORY_SIZE / 2
#define FRAME_ARENA_SIZE GAME_MEMORY_SIZE / 2

static b32 reload_game_code_if_recompiled(String so_path, Timestamp load_time, GameCode *game_code,
    LinearArena *frame_arena)
{
    Timestamp so_mod_time = platform_get_file_info(so_path, frame_arena).last_modification_time;

    if (timestamp_less_than(load_time, so_mod_time)) {
        unload_game_code(game_code);
        load_game_code(game_code, so_path, frame_arena);

        return true;
    }

    return false;
}

int main()
{
    // TODO: make this use mmap
    LinearArena main_arena = la_create(default_allocator, GAME_MEMORY_SIZE);

    GameMemory game_memory = {
        .permanent_memory = la_create(la_allocator(&main_arena), PERMANENT_ARENA_SIZE),
        .temporary_memory = la_create(la_allocator(&main_arena), FRAME_ARENA_SIZE)
    };

    GameState game_state = {0};

    run_tests();

    WindowHandle *window = platform_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "foo",
	WINDOW_FLAG_NON_RESIZABLE, la_allocator(&game_memory.permanent_memory));
    RendererBackend *backend = renderer_backend_initialize(la_allocator(&game_memory.permanent_memory));

    AssetManager assets = assets_initialize(la_allocator(&game_memory.permanent_memory));

    AssetList asset_list = {
        .shader = assets_register_shader(&assets, str_lit("shader.glsl"), &game_memory.temporary_memory),
        .shader2 = assets_register_shader(&assets, str_lit("shader2.glsl"), &game_memory.temporary_memory),
        .texture = assets_register_texture(&assets, str_lit("test.png"), &game_memory.temporary_memory),
        .white_texture = assets_register_texture(&assets, str_lit("white.png"), &game_memory.temporary_memory),
    };

    String executable_dir = platform_get_executable_directory(la_allocator(&game_memory.permanent_memory),
        &game_memory.temporary_memory);
    String so_path = str_concat(executable_dir, str_lit("/"GAME_SO_NAME), la_allocator(&game_memory.permanent_memory));

    GameCode game_code = {0};
    load_game_code(&game_code, so_path, &game_memory.temporary_memory);

    Input input = {0};

    Timestamp game_so_load_time = platform_get_time();

    AssetWatcherContext asset_watcher = {0};
    file_watcher_start(&asset_watcher);

    f32 time_point_last = platform_get_seconds_since_launch();
    f32 time_point_new = time_point_last;

    platform_set_scroll_value_storage(&input.scroll_delta, window);

    game_code.initialize(&game_state, &game_memory);

    RenderBatchList render_batches = {0};

    while (!platform_window_should_close(window)) {
        time_point_new = platform_get_seconds_since_launch();
        f32 dt = time_point_new - time_point_last;
        time_point_last = time_point_new;

        la_reset(&game_memory.temporary_memory);

        file_watcher_reload_modified_assets(&asset_watcher, &assets, &game_memory.temporary_memory);

        if (reload_game_code_if_recompiled(so_path, game_so_load_time, &game_code, &game_memory.temporary_memory)) {
            game_so_load_time = platform_get_time();
            printf("Hot reloaded game.\n");
        }

        platform_update_input(&input, window);

        renderer_backend_clear(backend);

        FrameData frame_data = {
            .dt = dt,
            .input = &input,
            .window_width = WINDOW_WIDTH,
            .window_height = WINDOW_HEIGHT,
        };

        list_clear(&render_batches);
        game_code.update_and_render(&game_state, &render_batches, &asset_list, frame_data, &game_memory);

        for (RenderBatchNode *node = list_head(&render_batches); node; node = list_next(node)) {
            execute_render_commands(&node->render_batch, &assets, backend, &game_memory.temporary_memory);
        }

        input.scroll_delta = 0.0f;
        platform_poll_events(window);
    }

    platform_destroy_window(window);
    la_destroy(&main_arena);

    file_watcher_stop(&asset_watcher);

    return 0;
}
