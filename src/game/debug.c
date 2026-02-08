#include "debug.h"
#include "base/format.h"
#include "base/linear_arena.h"
#include "components/component.h"
#include "entity/entity_faction.h"
#include "entity/entity_system.h"
#include "platform/input.h"
#include "status_effect.h"
#include "game.h"
#include "renderer/frontend/render_batch.h"

void render_quad_tree(QuadTreeNode *tree, RenderBatch *rb, LinearArena *arena, ssize depth)
{
    if (tree) {
	render_quad_tree(tree->top_left, rb, arena, depth + 1);
	render_quad_tree(tree->top_right, rb, arena, depth + 1);
	render_quad_tree(tree->bottom_right, rb, arena, depth + 1);
	render_quad_tree(tree->bottom_left, rb, arena, depth + 1);

	static const RGBA32 colors[] = {
	    {1.0f, 0.0f, 0.0f, 0.5f},
	    {0.0f, 1.0f, 0.0f, 0.5f},
	    {0.0f, 0.0f, 1.0f, 0.5f},
	    {1.0f, 0.0f, 1.0f, 0.5f},
	    {0.2f, 0.3f, 0.8f, 0.5f},
	    {1.0f, 0.1f, 0.5f, 0.5f},
	    {0.5f, 0.5f, 0.5f, 0.5f},
	    {0.5f, 1.0f, 0.5f, 0.5f},
	};

	ASSERT(depth < ARRAY_COUNT(colors));

	RGBA32 color = colors[depth];
	draw_outlined_rectangle(rb, arena, tree->area, color, 4.0f, shader_handle(SHAPE_SHADER), 0);
    }
}

typedef struct {
    LinearArena *arena;
    UIState *ui;
} EffectListContext;

static StatusEffectCallbackResult status_effect_list_callback(StatusEffectCallbackParams params,
    void *user_data)
{
    EffectListContext context = *(EffectListContext *)user_data;

    String status_effect_name = status_effect_to_string(params.status_effect_id);
    String str = format(context.arena,
        FMT_STR" (%.2f)##%p",
        FMT_STR_ARG(status_effect_name),
        (f64)params.instance->time_remaining,
        (void *)params.instance);

    ui_selectable(context.ui, str);

    return STATUS_EFFECT_CALLBACK_PROCEED;
}

static void inspected_entity_debug_ui(UIState *ui, Game *game, LinearArena *scratch)
{
    // TODO: allow locking on to entity
    EntityID inspected_entity_id = game->game_ui.hovered_entity;
    Entity *entity = es_try_get_entity(&game->world.entity_system, inspected_entity_id);

    if (!entity) {
	return;
    }

    PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);
    ASSERT(physics && "How did you inspect a non-spatial entity?");

    // TODO: don't require name for containers
    ui_begin_container(ui, str_lit("inspect"), V2_ZERO, RGBA32_GREEN, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f); {
	// TODO: inventory and equipment
        String faction = entity_faction_to_string(entity->faction);

        String entity_str = format(scratch, "Hovered entity: %d", game->game_ui.hovered_entity.index);
        String entity_pos_str = format(scratch, "Position: "FMT_V2, FMT_V2_ARG(physics->position));
        String entity_faction_str = format(scratch, "Faction: "FMT_STR,
            FMT_STR_ARG(faction));

	ui_text(ui, entity_str);
	ui_text(ui, entity_pos_str);
	ui_text(ui, entity_faction_str);

	HealthComponent *hp = es_get_component(entity, HealthComponent);

	if (hp) {
            String hp_str = format(scratch, "HP: %zu/%zu",
                hp->health.current_hitpoints, hp->health.max_hitpoints);

	    ui_text(ui, hp_str);
	}

	StatusEffectComponent *effects = es_get_component(entity, StatusEffectComponent);

	if (effects) {
	    ui_begin_list(ui, str_lit("status_effects")); {
		EffectListContext context = {0};
		context.arena = scratch;
		context.ui = ui;

		for_each_active_status_effect(effects, status_effect_list_callback, &context);
	    } ui_end_list(ui);
	}
    } ui_pop_container(ui);
}

static String dbg_arena_usage_string(String name, ssize usage, LinearArena *arena)
{
    String result = format(arena, FMT_STR": %.2f KBs", FMT_STR_ARG(name), (f64)usage / 1024.0);

    return result;
}

void debug_ui(UIState *ui, Game *game, LinearArena *scratch, const FrameData *frame_data)
{
    ui_begin_container(ui, str_lit("root"), V2_ZERO, RGBA32_TRANSPARENT, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);

    ssize temp_arena_memory_usage = game->debug_state.scratch_arena_memory_usage;
    ssize perm_arena_memory_usage = game->debug_state.permanent_arena_memory_usage;
    ssize world_arena_memory_usage = game->debug_state.world_arena_memory_usage;

    String frame_time_str = format(scratch, "Frame time: %.5f", (f64)frame_data->dt);
    String fps_str = format(scratch, "FPS: %.2f", (f64)game->debug_state.average_fps);

    String temp_arena_str = dbg_arena_usage_string(str_lit("Frame arena"), temp_arena_memory_usage, scratch);
    String perm_arena_str = dbg_arena_usage_string(str_lit("Permanent arena"), perm_arena_memory_usage, scratch);
    String world_arena_str = dbg_arena_usage_string(str_lit("World arena"), world_arena_memory_usage, scratch);

    ssize qt_nodes = qt_get_node_count(&game->world.quad_tree);
    String node_string = format(scratch, "Quad tree nodes: %ld", qt_nodes);

    String entity_string = format(scratch, "Alive entity count: %d", game->world.alive_entity_count);

    f32 timestep = game->debug_state.timestep_modifier;
    String timestep_value_str = {0};

    if (timestep == 0.0f) {
        timestep_value_str = str_lit("SINGLE STEP");
    } else {
        timestep_value_str = format(scratch, "%.2f", (f64)timestep);
    }

    String timestep_str = format(scratch, "Timestep modifier: "FMT_STR,
        FMT_STR_ARG(timestep_value_str));

    ui_text(ui, frame_time_str);
    ui_text(ui, fps_str);
    ui_text(ui, timestep_str);

    ui_spacing(ui, 8);

    ui_text(ui, temp_arena_str);
    ui_text(ui, perm_arena_str);
    ui_text(ui, world_arena_str);
    ui_text(ui, node_string);
    ui_text(ui, entity_string);

    ui_spacing(ui, 8);

    ui_checkbox(ui, str_lit("Render quad tree"),         &game->debug_state.quad_tree_overlay);
    ui_checkbox(ui, str_lit("Render colliders"),         &game->debug_state.render_colliders);
    ui_checkbox(ui, str_lit("Render origin"),            &game->debug_state.render_origin);
    ui_checkbox(ui, str_lit("Render entity bounds"),     &game->debug_state.render_entity_bounds);
    ui_checkbox(ui, str_lit("Render entity velocity"),   &game->debug_state.render_entity_velocity);
    ui_checkbox(ui, str_lit("Render edge list"),         &game->debug_state.render_edge_list);

    ui_spacing(ui, 8);

    {
        String camera_pos_str = format(scratch, "Camera position: "FMT_V2,
            FMT_V2_ARG(game->world.camera.position));
        String zoom_str = format(scratch, "Zoom: %.2f", (f64)game->world.camera.zoom);

        ui_text(ui, camera_pos_str);
        ui_text(ui, zoom_str);
    }

    ui_spacing(ui, 8);
    inspected_entity_debug_ui(ui, game, scratch);

    ui_pop_container(ui);
}

void debug_update(Game *game, const FrameData *frame_data, LinearArena *frame_arena)
{
    f32 curr_fps = 1.0f / frame_data->dt;
    f32 avg_fps = game->debug_state.average_fps;

    // NOTE: lower alpha value means more smoothing
    game->debug_state.average_fps = exponential_moving_avg(avg_fps, curr_fps, 0.9f);

    Vector2 hovered_coords = screen_to_world_coords(
        game->world.camera,
        frame_data->input.mouse_position,
        frame_data->window_size
    );

    Rectangle hovered_rect = {hovered_coords, {1, 1}};
    EntityIDList hovered_entities = qt_get_entities_in_area(&game->world.quad_tree,
	hovered_rect, frame_arena);

    if (!list_is_empty(&hovered_entities)) {
        game->game_ui.hovered_entity = list_head(&hovered_entities)->id;
    } else {
        game->game_ui.hovered_entity = NULL_ENTITY_ID;
    }

    f32 speed_modifier = game->debug_state.timestep_modifier;
    f32 speed_modifier_step = 0.25f;

    if (speed_modifier < 0.5f) {
        speed_modifier_step = 0.05f;
    }

    if (input_is_key_pressed(&frame_data->input, KEY_UP)) {
        speed_modifier += speed_modifier_step;
    } else if (input_is_key_pressed(&frame_data->input, KEY_DOWN)) {
        if (speed_modifier - 0.05f <= 0.5f) {
            speed_modifier_step = 0.05f;
        }

        speed_modifier -= speed_modifier_step;
    }

    if (input_is_key_pressed(&frame_data->input, KEY_G)) {
        if (game->debug_state.timestep_modifier > 0.0f) {
            // Enter single stepping mode
            speed_modifier = 0.0f;
        } else {
            //Exit single stepping mode
            speed_modifier = 1.0f;
        }
    }

    if (input_is_key_pressed(&frame_data->input, KEY_T)) {
        game->debug_state.debug_menu_active = !game->debug_state.debug_menu_active;
    }

    game->debug_state.timestep_modifier = CLAMP(speed_modifier, 0.0f, 5.0f);
}
