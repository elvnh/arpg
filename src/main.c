#include <GLFW/glfw3.h>
#include <pthread.h>

#include "asset_table.h"
#include "platform/asset_system.h"
#include "platform/platform.h"
#include "platform/file_watcher.h"
#include "platform/hot_reload.h"
#include "renderer/backend/render_command_interpreter.h"
#include "game/game.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 768

// If hot reloading is enabled, we call any game functions through the function pointers
// loaded from the shared library. Otherwise, we just call the functions directly.
#if HOT_RELOAD
#    define GAME_INITIALIZE(game, mem, gc) (gc).initialize(game, mem);
#    define GAME_UPDATE_AND_RENDER(game, pf_code, rbs, frame_data, mem, gc)     \
        (gc).update_and_render(game, pf_code, rbs, frame_data, mem);
#    define HOT_RELOAD_IF_RECOMPILED(gc, mem) reload_game_code_if_recompiled(gc, mem)
#else
#    define GAME_INITIALIZE(game, mem, gc) game_initialize(game_state, game_memory);
#    define GAME_UPDATE_AND_RENDER(game, pf_code, rbs, frame_data, mem, gc) \
        game_update_and_render(game, pf_code, rbs, frame_data, mem);
#endif

const char *__asan_default_options(void) { return "detect_leaks=0"; }

int main(void)
{
    LinearArena main_arena = la_create(default_allocator, GAME_MEMORY_SIZE);

    GameMemory game_memory = {0};

    game_memory.permanent_memory = la_create(la_allocator(&main_arena), PERMANENT_ARENA_SIZE);
    game_memory.temporary_memory = la_create(la_allocator(&main_arena), FRAME_ARENA_SIZE);
    game_memory.free_list_memory = fl_create(la_allocator(&game_memory.permanent_memory), FREE_LIST_ARENA_SIZE);

    Game *game_state = la_allocate_item(&game_memory.permanent_memory, Game);

    platform_trap_on_fp_exceptions();

    WindowHandle *window = platform_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "foo",
	WINDOW_FLAG_NON_RESIZABLE, la_allocator(&game_memory.permanent_memory));
    RendererBackend *backend = renderer_backend_initialize(v2i(WINDOW_WIDTH, WINDOW_HEIGHT),
        la_allocator(&game_memory.permanent_memory));

    assets_initialize(la_allocator(&game_memory.permanent_memory));

    u64 rng_seed = (u64)time(0); // TODO: better seed
    rng_initialize(&game_state->rng_state, rng_seed);

    game_state->asset_table = load_game_assets(&game_memory.temporary_memory);

    // Function pointers to platform layer code that need to be called from game layer
    PlatformCode platform_code = {
        .get_text_dimensions = assets_get_text_dimensions,
        .get_text_newline_advance = assets_get_text_newline_advance,
        .get_font_baseline_offset = assets_get_font_baseline_offset,
    };

#if HOT_RELOAD
    GameCode game_code = hot_reload_initialize(&game_memory);
#endif

    Input input = {0};
    platform_initialize_input(&input, window);

    AssetWatcherContext asset_watcher = {0};
    file_watcher_start(&asset_watcher);

    f32 time_point_last = platform_get_seconds_since_launch();
    f32 time_point_new = time_point_last;

    GAME_INITIALIZE(game_state, &game_memory, game_code);

    RenderBatchList render_batches = {0};

    while (!platform_window_should_close(window)) {
        time_point_new = platform_get_seconds_since_launch();
        f32 dt = time_point_new - time_point_last;
        time_point_last = time_point_new;

        la_reset(&game_memory.temporary_memory);

        // TODO: Guard asset reloading behind macro too, just like code hot reloading
        file_watcher_reload_modified_assets(&asset_watcher, &game_memory.temporary_memory);
        HOT_RELOAD_IF_RECOMPILED(&game_code, &game_memory.temporary_memory);

        platform_update_input(&input, window);

        Vector2i window_size = platform_get_window_size(window);
        FrameData frame_data = {
            .dt = dt,
            .input = input,
            .window_size = window_size
        };

        list_clear(&render_batches);

        GAME_UPDATE_AND_RENDER(game_state, platform_code, &render_batches, frame_data, &game_memory, game_code);

        renderer_backend_begin_frame(backend);

        for (RenderBatch *batch = list_head(&render_batches); batch; batch = list_next(batch)) {
            execute_render_commands(batch, backend, &game_memory.temporary_memory);
        }

        renderer_backend_set_stencil_function(backend, STENCIL_FUNCTION_ALWAYS, 0);

        // TODO: Do this somewhere else
        ShaderAsset *light_blending_shader = assets_get_shader(
            get_shader_handle_from_table(&game_state->asset_table, ASSET_LIGHT_BLENDING_SHADER));

        renderer_backend_blend_framebuffers(backend, FRAME_BUFFER_GAMEPLAY, FRAME_BUFFER_LIGHTING,
            light_blending_shader);

        ShaderAsset *screenspace_texture_shader = assets_get_shader(
            get_shader_handle_from_table(&game_state->asset_table, ASSET_SCREENSPACE_TEXTURE_SHADER));

        renderer_backend_draw_framebuffer_as_texture(backend, FRAME_BUFFER_OVERLAY, screenspace_texture_shader);

        platform_poll_events(window);
    }

    platform_destroy_window(window);
    la_destroy(&main_arena);

    file_watcher_stop(&asset_watcher);

    return 0;
}
