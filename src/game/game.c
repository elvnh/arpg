#include "game.h"
#include "base/format.h"
#include "base/matrix.h"
#include "base/random.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "renderer/render_target.h"
#include "renderer/renderer_backend.h"
#include "status_effect.h"
#include "entity_id.h"
#include "entity_system.h"
#include "item_system.h"
#include "modifier.h"
#include "renderer/render_batch.h"
#include "platform/input.h"
#include "renderer/render_key.h"
#include "ui/ui_builder.h"
#include "ui/widget.h"
#include "world.h"

typedef enum {
    UI_OVERLAY_GAME,
    UI_OVERLAY_DEBUG,
} UIOverlayType;

static void set_global_pointers(Game *game)
{
    rng_set_global_state(&game->rng_state);
    magic_set_global_spell_array(&game->spells);
    anim_set_global_animation_table(&game->animations);
    set_global_asset_table(&game->asset_list);

    magic_initialize(&game->spells);
    anim_initialize(&game->animations);
}

static RenderBatches create_render_batches(Game *game, RenderBatchList *rbs,
    const FrameData *frame_data, LinearArena *scratch)
{
    RenderBatches result = {0};
    Camera ui_camera = create_screenspace_camera(frame_data->window_size);

    // TODO: make ambient light be property of each world
    f32 x = 0.2f;
    RGBA32 ambient_light = {x, x, x + 0.1f, 1.0f};

    result.world_rb = rb_list_push_new(rbs, game->world.camera, frame_data->window_size,
        Y_IS_UP, FRAME_BUFFER_GAMEPLAY, RGBA32_TRANSPARENT,
        BLEND_FUNCTION_MULTIPLICATIVE, scratch);

    result.lighting_rb = rb_list_push_new(rbs, game->world.camera, frame_data->window_size,
        Y_IS_UP, FRAME_BUFFER_LIGHTING, ambient_light,
        BLEND_FUNCTION_ADDITIVE, scratch);

    result.lighting_stencil_rb = rb_add_stencil_pass(result.lighting_rb,
        STENCIL_FUNCTION_NOT_EQUAL, 1, STENCIL_OP_REPLACE, scratch);

    result.worldspace_ui_rb = rb_list_push_new(rbs, game->world.camera, frame_data->window_size,
        Y_IS_UP, FRAME_BUFFER_OVERLAY, RGBA32_TRANSPARENT,
        BLEND_FUNCTION_MULTIPLICATIVE, scratch);

    result.overlay_rb = rb_list_push_new(rbs, ui_camera, frame_data->window_size,
        Y_IS_DOWN, FRAME_BUFFER_OVERLAY, RGBA32_TRANSPARENT,
        BLEND_FUNCTION_MULTIPLICATIVE, scratch);

    return result;
}

static void render_overlay_ui(UIState *ui, Game *game, UIOverlayType overlay, FrameData *frame_data,
    LinearArena *scratch, RenderBatch *rb, PlatformCode platform_code)
{
    ui_core_begin_frame(ui);

    switch (overlay) {
        case UI_OVERLAY_GAME: {
            game_ui(game, scratch, frame_data);
        } break;

        case UI_OVERLAY_DEBUG: {
            debug_ui(ui, game, scratch, frame_data);
        } break;

        INVALID_DEFAULT_CASE;
    }

    UIInteraction interaction = ui_core_end_frame(ui, frame_data, rb, platform_code);

    // TODO: shouldn't click_began_inside_ui be counted as receiving mouse input?
    if (interaction.received_mouse_input || interaction.click_began_inside_ui) {
        input_consume_input(&frame_data->input, MOUSE_LEFT);
    }
}

static void game_render(Game *game, RenderBatchList *rb_list, FrameData *frame_data,
    LinearArena *frame_arena, PlatformCode platform_code)
{
    RenderBatches rbs = create_render_batches(game, rb_list, frame_data, frame_arena);

    world_render(&game->world, rbs, frame_data, frame_arena, &game->debug_state);

    render_overlay_ui(&game->game_ui.backend_state, game, UI_OVERLAY_GAME, frame_data, frame_arena,
        rbs.overlay_rb, platform_code);

    if (game->debug_state.debug_menu_active) {
        render_overlay_ui(&game->debug_state.debug_ui, game, UI_OVERLAY_DEBUG, frame_data, frame_arena,
            rbs.overlay_rb, platform_code);
    }

    if (game->debug_state.quad_tree_overlay) {
        render_quad_tree(&game->world.quad_tree.root, rbs.worldspace_ui_rb, frame_arena, 0);
    }

    if (game->debug_state.render_origin) {
        rb_push_rect(rbs.worldspace_ui_rb, frame_arena, (Rectangle){{0, 0}, {8, 8}}, RGBA32_RED,
	    get_asset_table()->shape_shader, 3);
    }

    for (RenderBatchNode *node = list_head(rb_list); node; node = list_next(node)) {
        rb_sort_entries(&node->render_batch, frame_arena);
    }
}

static void game_update(Game *game, FrameData *frame_data, LinearArena *frame_arena)
{
    ASSERT(game->debug_state.timestep_modifier >= 0.0f);

#if HOT_RELOAD
    // NOTE: these global pointers are set every frame in case we have hot reloaded
    set_global_pointers(game);
#endif

    debug_update(game, frame_data, frame_arena);
    frame_data->dt *= game->debug_state.timestep_modifier;

    b32 game_paused = game->debug_state.timestep_modifier == 0.0f;
    b32 frame_advance_key_pressed = input_is_key_pressed(&frame_data->input, KEY_K);
    b32 should_update = !game_paused || frame_advance_key_pressed;

    // TODO: allow moving camera even while paused
    if (should_update) {
        if (game_paused) {
            // When advancing by a single frame, set the dt to a reasonable default value
            frame_data->dt = 0.016f;
        }

        world_update(&game->world, frame_data, frame_arena, &game->game_ui);
    }
}

// TODO: only send FrameData into this function, send dt etc to others
void game_update_and_render(Game *game, PlatformCode platform_code, RenderBatchList *rbs,
    FrameData frame_data, GameMemory *game_memory)
{
    if (input_is_key_pressed(&frame_data.input, KEY_ESCAPE)) {
        DEBUG_BREAK;
    }

    game_update(game, &frame_data, &game_memory->temporary_memory);
    game_render(game, rbs, &frame_data, &game_memory->temporary_memory, platform_code);

    // NOTE: These stats are set at end of frame since debug UI is drawn before the arenas
    // have had time to be used during the frame. This means that the stats have 1 frame delay
    // but that really doesn't matter
    game->debug_state.scratch_arena_memory_usage = la_get_memory_usage(&game_memory->temporary_memory);
    game->debug_state.permanent_arena_memory_usage = la_get_memory_usage(&game_memory->permanent_memory);
    game->debug_state.world_arena_memory_usage = fl_get_memory_usage(&game->world.world_arena);
}

void game_initialize(Game *game, GameMemory *game_memory)
{
    set_global_pointers(game);

    magic_initialize(&game->spells);
    anim_initialize(&game->animations);

    initialize_status_effect_system();
    es_initialize(&game->entity_system);
    item_sys_initialize(&game->item_system, la_allocator(&game_memory->permanent_memory));


    world_initialize(&game->world, &game->entity_system, &game->item_system, &game_memory->free_list_memory);

    game->debug_state.average_fps = 60.0f;
    game->debug_state.timestep_modifier = 1.0f;

    UIStyle default_ui_style = {
        .font = game->asset_list.default_font
    };

    // Debug UI
    ui_core_initialize(&game->debug_state.debug_ui, default_ui_style, &game_memory->permanent_memory);
    ui_core_initialize(&game->game_ui.backend_state, default_ui_style, &game_memory->permanent_memory);
}
