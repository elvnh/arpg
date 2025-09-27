#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#include "asset.h"
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
#include "game/renderer/render_batch.h"
#include "game/game.h"
#include "tests.c"
#include "file_watcher.h"
#include "hot_reload.h"
#include "renderer_dispatch.h"

#define WINDOW_WIDTH 768
#define WINDOW_HEIGHT 468

#define GAME_MEMORY_SIZE MB(32)
#define PERMANENT_ARENA_SIZE GAME_MEMORY_SIZE / 2
#define FRAME_ARENA_SIZE GAME_MEMORY_SIZE / 2

#define GAME_SO_NAME "libgame.so"

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
        .default_font = assets_register_font(&assets, str_lit("Ubuntu-M.ttf"), &game_memory.temporary_memory)
    };

    String executable_dir = platform_get_executable_directory(la_allocator(&game_memory.permanent_memory),
        &game_memory.temporary_memory);
    String so_path = str_concat(executable_dir, str_lit("/"GAME_SO_NAME), la_allocator(&game_memory.permanent_memory));

    GameCode game_code = {
        .game_library_path = str_null_terminate(so_path, la_allocator(&game_memory.permanent_memory)),
    };

    load_game_code(&game_code, &game_memory.temporary_memory);

    Input input = {0};

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

        reload_game_code_if_recompiled(&game_code, &game_memory.temporary_memory);

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
