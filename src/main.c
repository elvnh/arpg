#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#include "platform/asset.h"
#include "base/free_list_arena.h"
#include "base/hash.h"
#include "base/linear_arena.h"
#include "base/random.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/string8.h"
#include "base/image.h"
#include "base/list.h"
#include "platform/font.h"
#include "renderer/frontend/render_key.h"
#include "platform/input.h"
#include "platform/platform.h"
#include "renderer/frontend/render_target.h"
#include "renderer/backend/renderer_backend.h"
#include "platform/asset_system.h"
#include "renderer/frontend/render_command.h"
#include "base/vertex.h"
#include "renderer/frontend/render_batch.h"
#include "game/game.h"
#include "tests.c"
#include "platform/file_watcher.h"
#include "platform/hot_reload.h"
#include "renderer/backend/render_command_interpreter.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 768

static AssetSystem g_asset_system;

const char *__asan_default_options(void) { return "detect_leaks=0"; }

static Vector2 get_text_dimensions(FontHandle font_handle, String text, s32 text_size)
{
    FontAsset *asset = assets_get_font(&g_asset_system, font_handle);
    Vector2 result = font_get_text_dimensions(asset, text, text_size);

    return result;
}

static f32 get_text_newline_advance(FontHandle font_handle, s32 text_size)
{
    FontAsset *asset = assets_get_font(&g_asset_system, font_handle);
    f32 result = font_get_newline_advance(asset, text_size);

    return result;
}

static f32 get_font_baseline_offset(FontHandle font_handle, s32 text_size)
{
    FontAsset *asset = assets_get_font(&g_asset_system, font_handle);
    f32 result = font_get_baseline_offset(asset, text_size);

    return result;
}

int main(void)
{
    run_tests();

    // TODO: make this use mmap
    LinearArena main_arena = la_create(default_allocator, GAME_MEMORY_SIZE);

    GameMemory game_memory = {0};

    game_memory.permanent_memory = la_create(la_allocator(&main_arena), PERMANENT_ARENA_SIZE);
    game_memory.temporary_memory = la_create(la_allocator(&main_arena), FRAME_ARENA_SIZE);
    game_memory.free_list_memory
	= fl_create(la_allocator(&game_memory.permanent_memory), FREE_LIST_ARENA_SIZE);

    Game *game_state = la_allocate_item(&game_memory.permanent_memory, Game);

    platform_trap_on_fp_exceptions();

    WindowHandle *window = platform_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "foo",
	WINDOW_FLAG_NON_RESIZABLE, la_allocator(&game_memory.permanent_memory));
    RendererBackend *backend = renderer_backend_initialize(v2i(WINDOW_WIDTH, WINDOW_HEIGHT),
        la_allocator(&game_memory.permanent_memory));

    assets_initialize(&g_asset_system, la_allocator(&game_memory.permanent_memory));

    u64 rng_seed = (u64)time(0); // TODO: better seed
    rng_initialize(&game_state->rng_state, rng_seed);

    game_state->asset_list = (AssetTable){
        .texture_shader = assets_register_shader(&g_asset_system, str_lit("shader.glsl"), &game_memory.temporary_memory),
        .shape_shader = assets_register_shader(&g_asset_system, str_lit("shader2.glsl"), &game_memory.temporary_memory),
	.light_shader = assets_register_shader(&g_asset_system, str_lit("light.glsl"), &game_memory.temporary_memory),
	.light_blending_shader = assets_register_shader(&g_asset_system, str_lit("light_blending.glsl"),
            &game_memory.temporary_memory),
	.screenspace_texture_shader = assets_register_shader(&g_asset_system, str_lit("screenspace_texture.glsl"),
            &game_memory.temporary_memory),
        .default_texture = assets_register_texture(&g_asset_system, str_lit("test.png"), &game_memory.temporary_memory),
        .fireball_texture = assets_register_texture(&g_asset_system, str_lit("fireball.png"), &game_memory.temporary_memory),
        .spark_texture = assets_register_texture(&g_asset_system, str_lit("spark.png"), &game_memory.temporary_memory),
        .default_font = assets_register_font(&g_asset_system, str_lit("Ubuntu-M.ttf"), &game_memory.temporary_memory),
        .player_idle1 = assets_register_texture(&g_asset_system, str_lit("player_idle1.png"), &game_memory.temporary_memory),
        .player_idle2 = assets_register_texture(&g_asset_system, str_lit("player_idle2.png"), &game_memory.temporary_memory),
        .player_walking1 = assets_register_texture(&g_asset_system, str_lit("player_walking1.png"), &game_memory.temporary_memory),
        .player_walking2 = assets_register_texture(&g_asset_system, str_lit("player_walking2.png"), &game_memory.temporary_memory),
        .floor_texture = assets_register_texture(&g_asset_system, str_lit("floor.png"), &game_memory.temporary_memory),
        .wall_texture = assets_register_texture(&g_asset_system, str_lit("wall.png"), &game_memory.temporary_memory),
	.ice_shard_texture = assets_register_texture(&g_asset_system, str_lit("ice_shard.png"), &game_memory.temporary_memory),
	.player_attack1 = assets_register_texture(&g_asset_system, str_lit("player_attack1.png"), &game_memory.temporary_memory),
	.player_attack2 = assets_register_texture(&g_asset_system, str_lit("player_attack2.png"), &game_memory.temporary_memory),
	.player_attack3 = assets_register_texture(&g_asset_system, str_lit("player_attack3.png"), &game_memory.temporary_memory),
	.blizzard_texture = assets_register_texture(&g_asset_system, str_lit("blizzard.png"), &game_memory.temporary_memory),
    };

    PlatformCode platform_code = {
        .get_text_dimensions = get_text_dimensions,
        .get_text_newline_advance = get_text_newline_advance,
        .get_font_baseline_offset = get_font_baseline_offset,
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

        // TODO: should this be reset here or in game?
        la_reset(&game_memory.temporary_memory);

        file_watcher_reload_modified_assets(&asset_watcher, &g_asset_system, &game_memory.temporary_memory);

#if HOT_RELOAD
        reload_game_code_if_recompiled(&game_code, &game_memory.temporary_memory);
#endif

        platform_update_input(&input, window);

        Vector2i window_size = platform_get_window_size(window);
        FrameData frame_data = {
            .dt = dt,
            .input = input,
            .window_size = window_size
        };

        list_clear(&render_batches);

#if HOT_RELOAD
        game_code.update_and_render(game_state, platform_code, &render_batches, frame_data, &game_memory);
#else
        game_update_and_render(game_state, platform_code, &render_batches, frame_data, &game_memory);
#endif

        renderer_backend_begin_frame(backend);

        for (RenderBatch *batch = list_head(&render_batches); batch; batch = list_next(batch)) {
            execute_render_commands(batch, &g_asset_system, backend, &game_memory.temporary_memory);
        }

        renderer_backend_set_stencil_function(backend, STENCIL_FUNCTION_ALWAYS, 0);

        // TODO: don't do this here, do it in game.c?
        ShaderAsset *light_blending_shader = assets_get_shader(&g_asset_system, game_state->asset_list.light_blending_shader);
        renderer_backend_blend_framebuffers(backend, FRAME_BUFFER_GAMEPLAY, FRAME_BUFFER_LIGHTING,
            light_blending_shader);

        ShaderAsset *screenspace_texture_shader = assets_get_shader(&g_asset_system,
            game_state->asset_list.screenspace_texture_shader);
        renderer_backend_draw_framebuffer_as_texture(backend, FRAME_BUFFER_OVERLAY, screenspace_texture_shader);

        input.scroll_delta = 0.0f;
        platform_poll_events(window);
    }

    platform_destroy_window(window);
    la_destroy(&main_arena);

    file_watcher_stop(&asset_watcher);

    return 0;
}
