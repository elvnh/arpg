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

static void set_global_state(Game *game)
{
    rng_set_global_state(&game->rng_state);
    set_global_asset_table(&game->asset_list);

    magic_initialize();
    anim_initialize();
}

void update_player(World *world, const FrameData *frame_data,
    GameUIState *game_ui)
{
    Entity *player = world_get_player_entity(world);

    ASSERT(player);

    Vector2 camera_target = rect_center(world_get_entity_bounding_box(player));
    camera_set_target(&world->camera, camera_target);

    if (player->state.kind != ENTITY_STATE_ATTACKING) {
#if 0
	f32 speed = 350.0f;

	if (input_is_key_held(&frame_data->input, KEY_LEFT_SHIFT)) {
	    speed *= 3.0f;
	}

	Vector2 acceleration = {0};

	if (input_is_key_down(&frame_data->input, KEY_W)) {
	    acceleration.y = 1.0f;
	} else if (input_is_key_down(&frame_data->input, KEY_S)) {
	    acceleration.y = -1.0f;
	}

	if (input_is_key_down(&frame_data->input, KEY_A)) {
	    acceleration.x = -1.0f;
	} else if (input_is_key_down(&frame_data->input, KEY_D)) {
	    acceleration.x = 1.0f;
	}

	acceleration = v2_norm(acceleration);

	if (!v2_eq(acceleration, V2_ZERO)) {
	    entity_try_transition_to_state(world, player, state_walking());
	} else {
	    entity_try_transition_to_state(world, player, state_idle());
	}

	f32 dt = frame_data->dt;
	Vector2 v = player->velocity;
	Vector2 p = player->position;

	acceleration = v2_mul_s(acceleration, speed);
	acceleration = v2_add(acceleration, v2_mul_s(v, -3.5f));

	Vector2 new_pos = v2_add(
	    v2_add(v2_mul_s(v2_mul_s(acceleration, dt * dt), 0.5f), v2_mul_s(v, dt)),
	    p
	);

	Vector2 new_velocity = v2_add(v2_mul_s(acceleration, dt), v);

	player->position = new_pos;
	player->velocity = new_velocity;
#else
	Vector2 direction = {0};

	if (input_is_key_down(&frame_data->input, KEY_W)) {
	    direction.y = 1.0f;
	} else if (input_is_key_down(&frame_data->input, KEY_S)) {
	    direction.y = -1.0f;
	}

	if (input_is_key_down(&frame_data->input, KEY_A)) {
	    direction.x = -1.0f;
	} else if (input_is_key_down(&frame_data->input, KEY_D)) {
	    direction.x = 1.0f;
	}

	direction = v2_norm(direction);
	entity_try_transition_to_state(world, player, state_walking(direction));
#endif

	if (input_is_key_held(&frame_data->input, MOUSE_LEFT)) {
            Vector2 mouse_pos = frame_data->input.mouse_position;
            mouse_pos = screen_to_world_coords(world->camera, mouse_pos, frame_data->window_size);

	    SpellCasterComponent *spellcaster = es_get_component(player, SpellCasterComponent);
	    SpellID selected_spell = get_spell_at_spellbook_index(
		spellcaster, game_ui->selected_spellbook_index);

	    StatValue cast_speed = get_total_stat_value(player, STAT_CAST_SPEED, world->item_system);
	    StatValue action_speed = get_total_stat_value(player, STAT_ACTION_SPEED, world->item_system);

	    StatValue total_cast_speed = apply_modifier(cast_speed, action_speed,
		NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE);

	    entity_transition_to_state(world, player,
		state_attacking(selected_spell, mouse_pos, total_cast_speed));
        }
    }

    if (input_is_key_pressed(&frame_data->input, MOUSE_LEFT)
	&& !entity_id_is_null(game_ui->hovered_entity)) {
	Entity *hovered_entity = es_get_entity(world->entity_system, game_ui->hovered_entity);
	GroundItemComponent *ground_item = es_get_component(hovered_entity, GroundItemComponent);

	if (ground_item) {
	    // TODO: make try_get_component be nullable, make get_component assert on failure to get
	    InventoryComponent *inv = es_get_component(player, InventoryComponent);
	    ASSERT(inv);

	    inventory_add_item(&inv->inventory, ground_item->item_id);
	    es_schedule_entity_for_removal(hovered_entity);
	}
    }
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

static void update_and_render_ui(Game *game, RenderBatches rbs, FrameData *frame_data,
    LinearArena *frame_arena, PlatformCode platform_code)
{
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
}

static void game_render(Game *game, RenderBatches rbs, RenderBatchList *rb_list, FrameData *frame_data,
    LinearArena *frame_arena)
{
    world_render(&game->world, rbs, frame_data, frame_arena, &game->debug_state);

    for (RenderBatchNode *node = list_head(rb_list); node; node = list_next(node)) {
        rb_sort_entries(&node->render_batch, frame_arena);
    }
}

static void game_update(Game *game, FrameData *frame_data, LinearArena *frame_arena)
{
    ASSERT(game->debug_state.timestep_modifier >= 0.0f);

#if HOT_RELOAD
    // NOTE: these global pointers are set every frame in case we have hot reloaded
    set_global_state(game);
#endif

    debug_update(game, frame_data, frame_arena);

    // NOTE: We update camera independent of timestep modifier
    camera_zoom(&game->world.camera, (s32)frame_data->input.scroll_delta);
    camera_update(&game->world.camera, frame_data->dt);

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

        update_player(&game->world, frame_data, &game->game_ui);
        world_update(&game->world, frame_data, frame_arena);
    }
}

// TODO: only send FrameData into this function, send dt etc to others
void game_update_and_render(Game *game, PlatformCode platform_code, RenderBatchList *rbs,
    FrameData frame_data, GameMemory *game_memory)
{
    if (input_is_key_pressed(&frame_data.input, KEY_ESCAPE)) {
        DEBUG_BREAK;
    }

    RenderBatches game_rbs = create_render_batches(game, rbs, &frame_data, &game_memory->temporary_memory);

    update_and_render_ui(game, game_rbs, &frame_data, &game_memory->temporary_memory, platform_code);

    game_update(game, &frame_data, &game_memory->temporary_memory);
    game_render(game, game_rbs, rbs, &frame_data, &game_memory->temporary_memory);

    // NOTE: These stats are set at end of frame since debug UI is drawn before the arenas
    // have had time to be used during the frame. This means that the stats have 1 frame delay
    // but that really doesn't matter
    game->debug_state.scratch_arena_memory_usage = la_get_memory_usage(&game_memory->temporary_memory);
    game->debug_state.permanent_arena_memory_usage = la_get_memory_usage(&game_memory->permanent_memory);
    game->debug_state.world_arena_memory_usage = fl_get_memory_usage(&game->world.world_arena);
}

void game_initialize(Game *game, GameMemory *game_memory)
{
    set_global_state(game);

    magic_initialize();
    anim_initialize();

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
