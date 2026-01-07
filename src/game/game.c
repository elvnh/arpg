#include "game.h"
#include "base/format.h"
#include "base/matrix.h"
#include "base/random.h"
#include "renderer/render_batch.h"
#include "input.h"

static void game_update(Game *game, const FrameData *frame_data, LinearArena *frame_arena)
{
    (void)frame_arena;

    if (input_is_key_pressed(&frame_data->input, KEY_ESCAPE)) {
        DEBUG_BREAK;
    }

    world_update(&game->world, frame_data, frame_arena, &game->debug_state);
}

static void render_tree(QuadTreeNode *tree, RenderBatch *rb, LinearArena *arena, ssize depth)
{
    if (tree) {
	render_tree(tree->top_left, rb, arena, depth + 1);
	render_tree(tree->top_right, rb, arena, depth + 1);
	render_tree(tree->bottom_right, rb, arena, depth + 1);
	render_tree(tree->bottom_left, rb, arena, depth + 1);

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
	rb_push_outlined_rect(rb, arena, tree->area, color, 4.0f, get_asset_table()->shape_shader, 2);
    }
}



static void game_render(Game *game, RenderBatchList *rbs, const FrameData *frame_data, LinearArena *frame_arena)
{
    RenderBatch *rb = rb_list_push_new(rbs, game->world.camera, frame_data->window_size, Y_IS_UP, frame_arena);

    world_render(&game->world, rb, frame_data, frame_arena, &game->debug_state);

    if (game->debug_state.quad_tree_overlay) {
        render_tree(&game->world.entity_system.quad_tree.root, rb, frame_arena, 0);
    }

    if (game->debug_state.render_origin) {
        rb_push_rect(rb, frame_arena, (Rectangle){{0, 0}, {8, 8}}, RGBA32_RED,
	    get_asset_table()->shape_shader, 3);
    }


#if 0
    Rectangle rect_a = {{0}, {128, 128}};
    Rectangle rect_b = {{32, 64}, {128, 256}};

    Rectangle overlap = rect_overlap_area(rect_a, rect_b);

    rb_push_rect(rb, frame_arena, rect_a, (RGBA32){0, 1, 0, 0.5f},
	game->asset_list.shape_shader, 3);
    rb_push_rect(rb, frame_arena, rect_b, (RGBA32){0, 0, 1, 0.5f},
	game->asset_list.shape_shader, 3);
    rb_push_clipped_sprite(rb, frame_arena, game->asset_list.default_texture, rect_a, overlap,
	RGBA32_WHITE, game->asset_list.texture_shader, 4);
#endif

    rb_sort_entries(rb, frame_arena);
}

static String dbg_arena_usage_string(String name, ssize usage, Allocator allocator)
{
    String result = str_concat(
        name,
        str_concat(
            f32_to_string((f32)usage / 1024.0f, 2, allocator),
            str_lit(" KBs"),
            allocator),
        allocator
    );

    return result;
}

static void debug_ui(UIState *ui, Game *game, GameMemory *game_memory, const FrameData *frame_data)
{
    ui_begin_container(ui, str_lit("root"), V2_ZERO, RGBA32_TRANSPARENT, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);

    ssize temp_arena_memory_usage = game->debug_state.scratch_arena_memory_usage;
    ssize perm_arena_memory_usage = game->debug_state.permanent_arena_memory_usage;
    ssize world_arena_memory_usage = game->debug_state.world_arena_memory_usage;

    Allocator temp_alloc = la_allocator(&game_memory->temporary_memory);

    String frame_time_str = str_concat(
        str_lit("Frame time: "),
        f32_to_string(frame_data->dt, 5, temp_alloc),
        temp_alloc
    );

    String fps_str = str_concat(
        str_lit("FPS: "),
        f32_to_string(game->debug_state.average_fps, 2, temp_alloc),
        temp_alloc
    );

    String temp_arena_str = dbg_arena_usage_string(str_lit("Frame arena: "), temp_arena_memory_usage, temp_alloc);
    String perm_arena_str = dbg_arena_usage_string(str_lit("Permanent arena: "), perm_arena_memory_usage, temp_alloc);
    String world_arena_str = dbg_arena_usage_string(str_lit("World arena: "), world_arena_memory_usage, temp_alloc);

    ssize qt_nodes = qt_get_node_count(&game->world.entity_system.quad_tree);
    String node_string = str_concat(str_lit("Quad tree nodes: "), ssize_to_string(qt_nodes, temp_alloc), temp_alloc);

    String entity_string = str_concat(
        str_lit("Alive entity count: "),
        ssize_to_string(game->world.entity_system.alive_entity_count, temp_alloc),
        temp_alloc
    );

    f32 timestep = game->debug_state.timestep_modifier;
    String timestep_str = str_concat(
        str_lit("Timestep modifier: "),
        f32_to_string(timestep, 2, temp_alloc),
        temp_alloc
    );

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

    ui_checkbox(ui, str_lit("Render quad tree"),     &game->debug_state.quad_tree_overlay);
    ui_checkbox(ui, str_lit("Render colliders"),     &game->debug_state.render_colliders);
    ui_checkbox(ui, str_lit("Render origin"),        &game->debug_state.render_origin);
    ui_checkbox(ui, str_lit("Render entity bounds"), &game->debug_state.render_entity_bounds);

    ui_spacing(ui, 8);

    {
        String camera_pos_str = str_concat(
            str_lit("Camera position: "),
            v2_to_string(game->world.camera.position, temp_alloc),
            temp_alloc
        );

        ui_text(ui, camera_pos_str);
        ui_text(ui, str_concat(str_lit("Zoom: "), f32_to_string(game->world.camera.zoom, 2, temp_alloc), temp_alloc));
    }

    ui_spacing(ui, 8);

    // TODO: display more stats about hovered entity
    if (!entity_id_equal(game->debug_state.hovered_entity, NULL_ENTITY_ID)) {
        Entity *entity = es_try_get_entity(&game->world.entity_system, game->debug_state.hovered_entity);

        if (entity) {
            String entity_str = str_concat(
                str_lit("Hovered entity: "),
                ssize_to_string(game->debug_state.hovered_entity.slot_id, temp_alloc),
                temp_alloc
            );

            String entity_pos_str = str_concat(
                str_lit("Position: "), v2_to_string(entity->position, temp_alloc), temp_alloc
            );

            String entity_faction_str = {0};
            switch (entity->faction) {
                case FACTION_NEUTRAL: {
                    entity_faction_str = str_lit("Neutral");
                } break;

                case FACTION_PLAYER: {
                    entity_faction_str = str_lit("Player");
                } break;

                case FACTION_ENEMY: {
                    entity_faction_str = str_lit("Enemy");
                } break;
            }

            entity_faction_str = str_concat(str_lit("Faction: "), entity_faction_str, temp_alloc);

            ui_text(ui, entity_str);
            ui_text(ui, entity_pos_str);
            ui_text(ui, entity_faction_str);

            if (es_has_component(entity, InventoryComponent)) {
                ui_spacing(ui, 8);

                InventoryComponent *inv = es_get_component(entity, InventoryComponent);
                ui_text(ui, str_lit("Inventory:"));

                for (ssize i = 0; i < inv->inventory.item_count; ++i) {
                    ItemID id = inv->inventory.items[i];

                    Item *item = item_mgr_get_item(&game->world.item_manager, id);
                    ASSERT(item);

                    String id_string = ssize_to_string((ssize)id.id, temp_alloc);
                    id_string = str_concat(id_string, str_lit(", "), temp_alloc);
                    ui_text(ui, id_string);
                }
            }

            if (es_has_component(entity, EquipmentComponent)) {
                ui_spacing(ui, 8);

                EquipmentComponent *eq = es_get_component(entity, EquipmentComponent);
                ui_text(ui, str_lit("Equipment:"));

                {
                    {
                        ui_text(ui, str_lit("Head: "));

                        if (has_item_equipped_in_slot(&eq->equipment, EQUIP_SLOT_HEAD)) {
                            ui_core_same_line(ui);

                            ItemID item_id = get_equipped_item_in_slot(&eq->equipment, EQUIP_SLOT_HEAD);
                            String item_str = ssize_to_string((ssize)item_id.id, temp_alloc);
                            ui_text(ui, item_str);
                        }

                    }
                }

            }
        }
    }

    ui_pop_container(ui);
}

static void debug_update(Game *game, const FrameData *frame_data, LinearArena *frame_arena)
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
    EntityIDList hovered_entities = es_get_entities_in_area(&game->world.entity_system,
	hovered_rect, frame_arena);

    if (!list_is_empty(&hovered_entities)) {
        game->debug_state.hovered_entity = list_head(&hovered_entities)->id;
    } else {
        game->debug_state.hovered_entity = NULL_ENTITY_ID;
    }

    f32 speed_modifier = game->debug_state.timestep_modifier;
    f32 speed_modifier_step = 0.25;

    if (input_is_key_pressed(&frame_data->input, KEY_UP)) {
        speed_modifier += speed_modifier_step;
    } else if (input_is_key_pressed(&frame_data->input, KEY_DOWN)) {
        speed_modifier -= speed_modifier_step;
    }

    game->debug_state.timestep_modifier = CLAMP(speed_modifier, speed_modifier_step, 5.0f);
}


// TODO: only send FrameData into this function, send dt etc to others
void game_update_and_render(Game *game, PlatformCode platform_code, RenderBatchList *rbs,
    FrameData frame_data, GameMemory *game_memory)
{
#if HOT_RELOAD
    // NOTE: these global pointers are set every frame in case we have hot reloaded.
    rng_set_global_state(&game->rng_state);
    magic_set_global_spell_array(&game->spells);
    anim_set_global_animation_table(&game->animations);
    set_global_asset_table(&game->asset_list);

    // NOTE: Spells and animations are re-initialized every frame so that they can be changed during runtime.
    magic_initialize(&game->spells);
    anim_initialize(&game->animations);
#endif

    debug_update(game, &frame_data, &game_memory->temporary_memory);

    Camera ui_camera = {
        .position = {(f32)frame_data.window_size.x / 2.0f, (f32)frame_data.window_size.y / 2.0f}
    };

    RenderBatch debug_ui_rb = rb_create(ui_camera, frame_data.window_size, Y_IS_DOWN);
    RenderBatch game_ui_rb = rb_create(ui_camera, frame_data.window_size, Y_IS_DOWN);

    // TODO: create function for these two UI updates
    // Debug UI
    {
        if (game->debug_state.debug_menu_active) {
            ui_core_begin_frame(&game->debug_state.debug_ui);

            debug_ui(&game->debug_state.debug_ui,
                game, game_memory, &frame_data);

            UIInteraction dbg_ui_interaction =
                ui_core_end_frame(&game->debug_state.debug_ui, &frame_data, &debug_ui_rb,
		    platform_code);

            // TODO: shouldn't click_began_inside_ui be counted as receiving mouse input?
            if (dbg_ui_interaction.received_mouse_input || dbg_ui_interaction.click_began_inside_ui) {
                input_consume_input(&frame_data.input, MOUSE_LEFT);
            }
        }
    }

    // Game UI
    {
        ui_core_begin_frame(&game->game_ui.backend_state);

        game_ui(game, game_memory, &frame_data);

	UIInteraction game_ui_interaction =
            ui_core_end_frame(&game->game_ui.backend_state, &frame_data, &game_ui_rb, platform_code);

        if (game_ui_interaction.received_mouse_input || game_ui_interaction.click_began_inside_ui) {
            input_consume_input(&frame_data.input, MOUSE_LEFT);
        }
    }

    frame_data.dt *= game->debug_state.timestep_modifier;

    game_update(game, &frame_data, &game_memory->temporary_memory);
    game_render(game, rbs, &frame_data, &game_memory->temporary_memory);

    if (input_is_key_pressed(&frame_data.input, KEY_T)) {
        game->debug_state.debug_menu_active = !game->debug_state.debug_menu_active;
    }

    rb_sort_entries(&game_ui_rb, &game_memory->temporary_memory);
    rb_sort_entries(&debug_ui_rb, &game_memory->temporary_memory);

    // Debug UI is rendered on top of game UI
    rb_list_push(rbs, &game_ui_rb, &game_memory->temporary_memory);
    rb_list_push(rbs, &debug_ui_rb, &game_memory->temporary_memory);

    // NOTE: These stats are set at end of frame since debug UI is drawn before the arenas
    // have had time to be used during the frame. This means that the stats have 1 frame delay
    // but that really doesn't matter
    game->debug_state.scratch_arena_memory_usage = la_get_memory_usage(&game_memory->temporary_memory);
    game->debug_state.permanent_arena_memory_usage = la_get_memory_usage(&game_memory->permanent_memory);
    game->debug_state.world_arena_memory_usage = la_get_memory_usage(&game->world.world_arena);
}

void game_initialize(Game *game, GameMemory *game_memory)
{
    set_global_asset_table(&game->asset_list);
    magic_initialize(&game->spells);
    world_initialize(&game->world, &game_memory->permanent_memory);
    anim_initialize(&game->animations);

    game->debug_state.average_fps = 60.0f;
    game->debug_state.timestep_modifier = 1.0f;

    UIStyle default_ui_style = {
        .font = game->asset_list.default_font
    };

    // Debug UI
    ui_core_initialize(&game->debug_state.debug_ui, default_ui_style, &game_memory->permanent_memory);
    ui_core_initialize(&game->game_ui.backend_state, default_ui_style, &game_memory->permanent_memory);
}
