#include "game.h"

#include "asset_table.h"
#include "base/format.h"
#include "base/list.h"
#include "base/matrix.h"
#include "base/random.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "command.h"
#include "components/component.h"
#include "components/inventory.h"
#include "components/modifier.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "platform/input.h"
#include "platform/input_event.h"
#include "renderer/backend/renderer_backend.h"
#include "renderer/frontend/render_batch.h"
#include "renderer/frontend/render_key.h"
#include "renderer/frontend/render_target.h"
#include "status_effect.h"
#include "ui/game_ui.h"
#include "ui/ui_builder.h"
#include "ui/widget.h"
#include "world/world.h"

typedef enum {
    UI_OVERLAY_GAME,
    UI_OVERLAY_DEBUG,
} UIOverlayType;

static void set_global_state(Game *game)
{
    rng_set_global_state(&game->rng_state);
    set_global_asset_table(&game->asset_table);

    magic_initialize();
    initialize_flipbook_animations();
}

World *get_active_world(Game *game)
{
    ASSERT(game->world_array.current_world_index >= 0);
    ASSERT(game->world_array.current_world_index < ARRAY_COUNT(game->world_array.data));

    World *result = &game->world_array.data[game->world_array.current_world_index];

    return result;
}

Camera *get_active_camera(Game *game)
{
    Camera *result = 0;

    if (game->debug_state.debug_camera_active) {
        result = &game->debug_state.debug_camera;
    } else {
        result = &get_active_world(game)->camera;
    }

    return result;
}

// TODO: make this return a CommandQueue instead
static void process_input(World *world, FrameInput *frame_input, GameUI *game_ui,
    Camera active_camera, CommandQueue *commands, LinearArena *arena)
{
    Entity *player = world_get_player_entity(world);
    ASSERT(player);
    PhysicsComponent *physics = es_get_component(player, PhysicsComponent);

    Vector2 camera_target = rect_center(world_get_entity_bounding_box(player, physics));
    camera_set_target(&world->camera, camera_target);

    Vector2 direction = {0};
    if (consume_key_down(&frame_input->input_events, KEY_W)) {
        direction.y = 1.0f;
    } else if (consume_key_down(&frame_input->input_events, KEY_S)) {
        direction.y = -1.0f;
    }

    if (consume_key_down(&frame_input->input_events, KEY_A)) {
        direction.x = -1.0f;
    } else if (consume_key_down(&frame_input->input_events, KEY_D)) {
        direction.x = 1.0f;
    }

    direction = v2_norm(direction);
    push_command(commands, move_command(direction), arena);

    b32 key_pressed = consume_key_pressed(&frame_input->input_events, MOUSE_LEFT);
    b32 key_held = consume_key_held(&frame_input->input_events, MOUSE_LEFT);
    b32 item_on_cursor = !entity_id_is_null(game_ui->inventory_menu.item_on_cursor);

    if (item_on_cursor && key_pressed) {
        Command cmd = item_transaction_command(game_ui->inventory_menu.item_on_cursor,
            item_location_cursor(), item_location_world());

        push_command(commands, cmd, arena);
    } else if (!item_on_cursor && (key_pressed || key_held)) {
        Vector2 mouse_pos = get_mouse_pos(&frame_input->input_events);
        Vector2 attack_target =
            screen_to_world_coords(active_camera, mouse_pos, frame_input->window_size);

        push_command(commands, attack_command(attack_target), arena);
    } else if (!entity_id_is_null(game_ui->hovered_entity)
               && consume_key_pressed(&frame_input->input_events, MOUSE_RIGHT)) {
        Command cmd = {0};

        if (game_ui->inventory_menu.active) {
            // If inventory is open, move item to cursor
            cmd = item_transaction_command(game_ui->hovered_entity, item_location_world(),
                item_location_cursor());
        } else {
            // Otherwise try to pick up in any available inventory slot
            cmd = item_transaction_command(game_ui->hovered_entity, item_location_world(),
                item_location_any_inventory_slot());
        }

        push_command(commands, cmd, arena);
    }
}

static RenderBatches create_render_batches(Game *game, RenderBatchList *rbs,
    FrameInput *frame_input, LinearArena *scratch)
{
    RenderBatches result = {0};
    Camera ui_camera = create_screenspace_camera(frame_input->window_size);

    // TODO: make ambient light be property of each world
    f32 x = 0.2f;
    RGBA32 ambient_light = {x, x, x + 0.1f, 1.0f};

    Camera active_camera = *get_active_camera(game);

    result.world_rb =
        push_new_render_batch(rbs, active_camera, frame_input->window_size, Y_IS_UP,
            FRAME_BUFFER_GAMEPLAY, RGBA32_TRANSPARENT, BLEND_FUNCTION_MULTIPLICATIVE, scratch);

    result.lighting_rb = push_new_render_batch(rbs, active_camera, frame_input->window_size,
        Y_IS_UP, FRAME_BUFFER_LIGHTING, ambient_light, BLEND_FUNCTION_ADDITIVE, scratch);

    result.lighting_stencil_rb = add_stencil_pass(result.lighting_rb,
        STENCIL_FUNCTION_NOT_EQUAL, 1, STENCIL_OP_REPLACE, scratch);

    result.worldspace_ui_rb =
        push_new_render_batch(rbs, active_camera, frame_input->window_size, Y_IS_UP,
            FRAME_BUFFER_OVERLAY, RGBA32_TRANSPARENT, BLEND_FUNCTION_MULTIPLICATIVE, scratch);

    result.overlay_rb =
        push_new_render_batch(rbs, ui_camera, frame_input->window_size, Y_IS_DOWN,
            FRAME_BUFFER_OVERLAY, RGBA32_TRANSPARENT, BLEND_FUNCTION_MULTIPLICATIVE, scratch);

    return result;
}

static void render_ui(Game *game, RenderBatches rbs, FrameInput *frame_input,
    LinearArena *frame_arena)
{
    World *world = get_active_world(game);

    ui_render(&game->game_ui.backend_state, frame_input, rbs.overlay_rb);

    if (game->debug_state.debug_menu_active) {
        ui_render(&game->debug_state.debug_ui, frame_input, rbs.overlay_rb);
    }

    if (game->debug_state.quad_tree_overlay) {
        debug_render_quad_tree(&world->quad_tree.root, rbs.worldspace_ui_rb, frame_arena, 0);
    }

    if (game->debug_state.render_origin) {
        draw_rectangle(rbs.worldspace_ui_rb, frame_arena,
            (Rectangle){
                {0, 0},
                {8, 8}
        },
            RGBA32_RED, shader_handle(SHAPE_SHADER), 3);
    }

    // TODO: move more debug rendering to debug file
    if (game->debug_state.render_chunks) {
        debug_render_chunks(game, rbs.worldspace_ui_rb, frame_arena);
    }
}

static void update_overlay_ui(UIState *ui, Game *game, UIOverlayType overlay,
    FrameInput *frame_input, CommandQueue *commands, LinearArena *scratch,
    PlatformCode platform_code)
{
    ui_begin_frame(ui);

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (overlay) {
        case UI_OVERLAY_GAME: {
            game_ui(game, scratch, &frame_input->input_events, commands);
        } break;

        case UI_OVERLAY_DEBUG: {
            debug_ui(ui, game, scratch, frame_input->dt);
        } break;

            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    ui_end_layout(ui, frame_input, Y_IS_DOWN, platform_code);
}

static void update_ui(Game *game, FrameInput *frame_input, CommandQueue *commands,
    LinearArena *frame_arena, PlatformCode platform_code)
{
    World *world = get_active_world(game);
    Vector2 mouse_pos = get_mouse_pos(&frame_input->input_events);
    Vector2 hovered_coords =
        screen_to_world_coords(world->camera, mouse_pos, frame_input->window_size);

    Rectangle hovered_rect = {
        hovered_coords, {1, 1}
    };

    EntityIDList hovered_entities =
        qt_get_entities_in_area(&world->quad_tree, hovered_rect, frame_arena);

    if (!list_is_empty(&hovered_entities)) {
        game->game_ui.hovered_entity = list_head(&hovered_entities)->id;
    } else {
        game->game_ui.hovered_entity = NULL_ENTITY_ID;
    }

    update_overlay_ui(&game->game_ui.backend_state, game, UI_OVERLAY_GAME, frame_input,
        commands, frame_arena, platform_code);

    if (game->debug_state.debug_menu_active) {
        update_overlay_ui(&game->debug_state.debug_ui, game, UI_OVERLAY_DEBUG, frame_input,
            commands, frame_arena, platform_code);
    }
}

static void game_render(Game *game, RenderBatchList *rb_list, FrameInput *frame_input,
    LinearArena *frame_arena)
{
    World *world = get_active_world(game);

    RenderBatches rbs = create_render_batches(game, rb_list, frame_input, frame_arena);

    world_render(world, rbs, frame_input->window_size, frame_arena, &game->debug_state);

    render_ui(game, rbs, frame_input, frame_arena);

    for (RenderBatch *batch = list_head(rb_list); batch; batch = list_next(batch)) {
        sort_render_entries(batch, frame_arena);
    }
}

static void game_update(Game *game, FrameInput *frame_input, PlatformCode platform_code,
    LinearArena *frame_arena)
{
    ASSERT(game->debug_state.timestep_modifier >= 0.0f);

    CommandQueue cmds = {0};
    debug_update(game, frame_input);
    update_ui(game, frame_input, &cmds, frame_arena, platform_code);

    World *world = get_active_world(game);
    Camera *active_camera = get_active_camera(game);

    camera_zoom(active_camera, (s32)consume_scroll_delta(&frame_input->input_events));

    // NOTE: normal camera is always updated too even if debug camera is active
    camera_update(&world->camera, frame_input->dt);

    frame_input->dt *= game->debug_state.timestep_modifier;

    b32 game_paused = game->debug_state.timestep_modifier == 0.0f;
    b32 frame_advance_key_pressed = consume_key_pressed(&frame_input->input_events, KEY_K);
    b32 should_update = !game_paused || frame_advance_key_pressed;

    if (should_update) {
        if (game_paused) {
            // When advancing by a single frame, set the dt to a reasonable default value
            frame_input->dt = 0.016f;
        }

        process_input(world, frame_input, &game->game_ui, *active_camera, &cmds, frame_arena);
        execute_command_queue(&cmds, world, &game->game_ui);

        world_update(world, frame_input->dt, frame_input->window_size, frame_arena);
    }
}

void game_update_and_render(Game *game, PlatformCode platform_code, RenderBatchList *rbs,
    FrameInput *frame_input, GameMemory *game_memory)
{
#if HOT_RELOAD
    // NOTE: these global pointers are set every frame in case we have hot reloaded
    set_global_state(game);
#endif

    if (consume_key_pressed(&frame_input->input_events, KEY_ESCAPE)) {
        DEBUG_BREAK;
    }

    game_update(game, frame_input, platform_code, &game_memory->temporary_memory);
    game_render(game, rbs, frame_input, &game_memory->temporary_memory);

    // NOTE: These stats are set at end of frame since debug UI is drawn before the arenas
    // have had time to be used during the frame. This means that the stats have 1 frame delay
    // but that really doesn't matter
    World *world = get_active_world(game);
    game->debug_state.scratch_arena_memory_usage =
        la_get_memory_usage(&game_memory->temporary_memory);
    game->debug_state.permanent_arena_memory_usage =
        la_get_memory_usage(&game_memory->permanent_memory);
    game->debug_state.world_arena_memory_usage = la_get_memory_usage(&world->world_arena);
}

void game_initialize(Game *game, GameMemory *game_memory)
{
    set_global_state(game);

    magic_initialize();
    initialize_flipbook_animations();
    initialize_status_effect_system();

    for (ssize i = 0; i < LEVEL_COUNT; ++i) {
        World *world = &game->world_array.data[i];
        world_initialize(world, &game_memory->free_list_memory);
    }

    game->debug_state.average_fps = 60.0f;
    game->debug_state.timestep_modifier = 1.0f;

    // NOTE: UI style is currently mostly unused
    UIStyle default_ui_style = {0};
    default_ui_style.font = font_handle(DEFAULT_FONT);
    default_ui_style.background_color = RGBA32_BLUE;
    default_ui_style.background_shadow_color = RGBA32_GRAY;
    default_ui_style.accent_color = RGBA32_GREEN;
    default_ui_style.context_menu_color = RGBA32_CYAN;
    default_ui_style.text_color = RGBA32_WHITE;

    // Debug UI
    ui_initialize(&game->debug_state.debug_ui, default_ui_style,
        &game_memory->permanent_memory);
    ui_initialize(&game->game_ui.backend_state, default_ui_style,
        &game_memory->permanent_memory);
}
