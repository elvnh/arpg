#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#include "asset.h"
#include "base/free_list_arena.h"
#include "base/hash.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/string8.h"
#include "base/image.h"
#include "base/list.h"
#include "font.h"
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

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 768

#define GAME_MEMORY_SIZE MB(32)
#define PERMANENT_ARENA_SIZE GAME_MEMORY_SIZE / 2
#define FRAME_ARENA_SIZE GAME_MEMORY_SIZE / 2

static AssetManager asset_mgr;

static Vector2 get_text_dimensions(FontHandle font_handle, String text, s32 text_size)
{
    FontAsset *asset = assets_get_font(&asset_mgr, font_handle);
    Vector2 result = font_get_text_dimensions(asset, text, text_size);
    //printf("%.2f, %.2f\n", (f64)result.x, (f64)result.y);

    return result;
}

int main()
{
    // TODO: make this use mmap
    LinearArena main_arena = la_create(default_allocator, GAME_MEMORY_SIZE);

    GameMemory game_memory = {
        .permanent_memory = la_create(la_allocator(&main_arena), PERMANENT_ARENA_SIZE),
        .temporary_memory = la_create(la_allocator(&main_arena), FRAME_ARENA_SIZE)
    };

    GameState *game_state = la_allocate_item(&game_memory.permanent_memory, GameState);

    run_tests();

    WindowHandle *window = platform_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "foo",
	WINDOW_FLAG_NON_RESIZABLE, la_allocator(&game_memory.permanent_memory));
    RendererBackend *backend = renderer_backend_initialize(la_allocator(&game_memory.permanent_memory));

    assets_initialize(&asset_mgr, la_allocator(&game_memory.permanent_memory));

    game_state->asset_list = (AssetList){
        .texture_shader = assets_register_shader(&asset_mgr, str_lit("shader.glsl"), &game_memory.temporary_memory),
        .shape_shader = assets_register_shader(&asset_mgr, str_lit("shader2.glsl"), &game_memory.temporary_memory),
        .default_texture = assets_register_texture(&asset_mgr, str_lit("test.png"), &game_memory.temporary_memory),
        .default_font = assets_register_font(&asset_mgr, str_lit("Ubuntu-M.ttf"), &game_memory.temporary_memory)
    };

    PlatformCode platform_code = {
        .get_text_dimensions = get_text_dimensions
    };

#if HOT_RELOAD
    GameCode game_code = hot_reload_initialize(&game_memory);
#endif

    Input input = {0};
    input_initialize(&input);

    AssetWatcherContext asset_watcher = {0};
    file_watcher_start(&asset_watcher);

    f32 time_point_last = platform_get_seconds_since_launch();
    f32 time_point_new = time_point_last;

    // TODO: do this some other way
    platform_set_scroll_value_storage(&input.scroll_delta, window);

#if HOT_RELOAD
    game_code.initialize(game_state, &game_memory);
#else
    game_initialize(game_state, &game_memory);
#endif

    RenderBatchList render_batches = {0};

    while (!platform_window_should_close(window)) {
        time_point_new = platform_get_seconds_since_launch();
        f32 dt = time_point_new - time_point_last;
        time_point_last = time_point_new;

        la_reset(&game_memory.temporary_memory);

        file_watcher_reload_modified_assets(&asset_watcher, &asset_mgr, &game_memory.temporary_memory);

#if HOT_RELOAD
        reload_game_code_if_recompiled(&game_code, &game_memory.temporary_memory);
#endif

        platform_update_input(&input, window);

        renderer_backend_clear(backend);

        Vector2i window_size = platform_get_window_size(window);
        FrameData frame_data = {
            .dt = dt,
            .input = &input,
            .window_size = window_size
        };

        list_clear(&render_batches);

#if HOT_RELOAD
        game_code.update_and_render(game_state, platform_code, &render_batches, frame_data, &game_memory);
#else
        game_update_and_render(game_state, platform_code, &render_batches, &asset_list, frame_data, &game_memory);
#endif

        for (RenderBatchNode *node = list_head(&render_batches); node; node = list_next(node)) {
            execute_render_commands(&node->render_batch, &asset_mgr, backend, &game_memory.temporary_memory);
        }

        input.scroll_delta = 0.0f;
        platform_poll_events(window);
    }

    platform_destroy_window(window);
    la_destroy(&main_arena);

    file_watcher_stop(&asset_watcher);

    return 0;
}
