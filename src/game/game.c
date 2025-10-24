#include "game.h"
#include "asset.h"
#include "base/allocator.h"
#include "base/format.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/list.h"
#include "base/matrix.h"
#include "base/rectangle.h"
#include "base/ring_buffer.h"
#include "base/sl_list.h"
#include "base/rgba.h"
#include "base/maths.h"
#include "base/string8.h"
#include "base/utils.h"
#include "base/vector.h"
#include "base/line.h"
#include "game/camera.h"
#include "game/component.h"
#include "game/damage.h"
#include "game/entity.h"
#include "game/entity_system.h"
#include "game/health.h"
#include "game/magic.h"
#include "game/quad_tree.h"
#include "game/renderer/render_key.h"
#include "game/tilemap.h"
#include "game/ui/ui_builder.h"
#include "game/ui/ui_core.h"
#include "game/ui/widget.h"
#include "input.h"
#include "collision.h"
#include "renderer/render_batch.h"

static void game_update(GameState *game_state, const Input *input, f32 dt, LinearArena *frame_arena)
{
    (void)frame_arena;

    if (input_is_key_pressed(input, KEY_ESCAPE)) {
        DEBUG_BREAK;
    }

    world_update(&game_state->world, input, dt, &game_state->asset_list, frame_arena);

}

static void render_tree(QuadTreeNode *tree, RenderBatch *rb, LinearArena *arena,
    const AssetList *assets, ssize depth)
{
    if (tree) {
	render_tree(tree->top_left, rb, arena, assets, depth + 1);
	render_tree(tree->top_right, rb, arena, assets, depth + 1);
	render_tree(tree->bottom_right, rb, arena, assets, depth + 1);
	render_tree(tree->bottom_left, rb, arena, assets, depth + 1);

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
	rb_push_outlined_rect(rb, arena, tree->area, color, 4.0f, assets->shape_shader, 2);
    }
}



static void game_render(GameState *game_state, RenderBatchList *rbs, FrameData frame_data, LinearArena *frame_arena)
{
    Matrix4 proj = camera_get_matrix(game_state->world.camera, frame_data.window_size);
    RenderBatch *rb = rb_list_push_new(rbs, proj, Y_IS_UP, frame_arena);

    world_render(&game_state->world, rb, &game_state->asset_list, frame_data, frame_arena,
	&game_state->debug_state);

    if (game_state->debug_state.quad_tree_overlay) {
        render_tree(&game_state->world.entities.quad_tree.root, rb, frame_arena, &game_state->asset_list, 0);
    }

    if (game_state->debug_state.render_origin) {
        rb_push_rect(rb, frame_arena, (Rectangle){{0, 0}, {8, 8}}, RGBA32_RED,
	    game_state->asset_list.shape_shader, 3);
    }


#if 0
    Rectangle rect_a = {{0}, {128, 128}};
    Rectangle rect_b = {{32, 64}, {128, 256}};

    Rectangle overlap = rect_overlap_area(rect_a, rect_b);

    rb_push_rect(rb, frame_arena, rect_a, (RGBA32){0, 1, 0, 0.5f},
	game_state->asset_list.shape_shader, 3);
    rb_push_rect(rb, frame_arena, rect_b, (RGBA32){0, 0, 1, 0.5f},
	game_state->asset_list.shape_shader, 3);
    rb_push_clipped_sprite(rb, frame_arena, game_state->asset_list.default_texture, rect_a, overlap,
	RGBA32_WHITE, game_state->asset_list.texture_shader, 4);
#endif

    rb_sort_entries(rb);
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

static void debug_ui(UIState *ui, GameState *game_state, GameMemory *game_memory, FrameData frame_data)
{
    ui_begin_container(ui, str_lit("root"), V2_ZERO, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);

    ssize temp_arena_memory_usage = la_get_memory_usage(&game_memory->temporary_memory);
    ssize perm_arena_memory_usage = la_get_memory_usage(&game_memory->permanent_memory);
    ssize world_arena_memory_usage = la_get_memory_usage(&game_state->world.world_arena);
    // TODO: asset memory usage

    Allocator temp_alloc = la_allocator(&game_memory->temporary_memory);

    String frame_time_str = str_concat(
        str_lit("Frame time: "),
        f32_to_string(frame_data.dt, 5, temp_alloc),
        temp_alloc
    );

    String fps_str = str_concat(
        str_lit("FPS: "),
        f32_to_string(game_state->debug_state.average_fps, 2, temp_alloc),
        temp_alloc
    );

    String temp_arena_str = dbg_arena_usage_string(str_lit("Frame arena: "), temp_arena_memory_usage, temp_alloc);
    String perm_arena_str = dbg_arena_usage_string(str_lit("Permanent arena: "), perm_arena_memory_usage, temp_alloc);
    String world_arena_str = dbg_arena_usage_string(str_lit("World arena: "), world_arena_memory_usage, temp_alloc);

    ssize qt_nodes = qt_get_node_count(&game_state->world.entities.quad_tree);
    String node_string = str_concat(str_lit("Quad tree nodes: "), ssize_to_string(qt_nodes, temp_alloc), temp_alloc);

    String entity_string = str_concat(
        str_lit("Alive entity count: "),
        ssize_to_string(game_state->world.entities.alive_entity_count, temp_alloc),
        temp_alloc
    );


    ui_text(ui, frame_time_str);
    ui_text(ui, fps_str);

    ui_spacing(ui, 8);

    ui_text(ui, temp_arena_str);
    ui_text(ui, perm_arena_str);
    ui_text(ui, world_arena_str);
    ui_text(ui, node_string);
    ui_text(ui, entity_string);

    ui_spacing(ui, 8);

    ui_checkbox(ui, str_lit("Render quad tree"),     &game_state->debug_state.quad_tree_overlay);
    ui_checkbox(ui, str_lit("Render colliders"),     &game_state->debug_state.render_colliders);
    ui_checkbox(ui, str_lit("Render origin"),        &game_state->debug_state.render_origin);
    ui_checkbox(ui, str_lit("Render entity bounds"), &game_state->debug_state.render_entity_bounds);

    ui_spacing(ui, 8);

    {
        String camera_pos_str = str_concat(
            str_lit("Camera position: "),
            v2_to_string(game_state->world.camera.position, temp_alloc),
            temp_alloc
        );

        ui_text(ui, camera_pos_str);
        ui_text(ui, str_concat(str_lit("Zoom: "), f32_to_string(game_state->world.camera.zoom, 2, temp_alloc), temp_alloc));
    }

    ui_spacing(ui, 8);

    // TODO: display more stats about hovered entity
    if (!entity_id_equal(game_state->debug_state.hovered_entity, NULL_ENTITY_ID)) {
        const Entity *entity = es_try_get_entity(&game_state->world.entities, game_state->debug_state.hovered_entity);

        if (entity) {
            String entity_str = str_concat(
                str_lit("Hovered entity: "),
                ssize_to_string(game_state->debug_state.hovered_entity.slot_id, temp_alloc),
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
        }
    }

    ui_button(ui, str_lit("ABCDEFG"));

    ui_pop_container(ui);
}

static void game_update_and_render_ui(UIState *ui, GameState *game_state, GameMemory *game_memory, FrameData frame_data)
{
    debug_ui(ui, game_state, game_memory, frame_data);
}

void debug_update(GameState *game_state, FrameData frame_data, LinearArena *frame_arena)
{
    f32 curr_fps = 1.0f / frame_data.dt;
    f32 avg_fps = game_state->debug_state.average_fps;

    // NOTE: lower alpha value means more smoothing
    game_state->debug_state.average_fps = exponential_moving_avg(avg_fps, curr_fps, 0.9f);

    Vector2 hovered_coords = screen_to_world_coords(
        game_state->world.camera,
        frame_data.input->mouse_position,
        frame_data.window_size
    );

    Rectangle hovered_rect = {hovered_coords, {1, 1}};
    EntityIDList hovered_entities = es_get_entities_in_area(&game_state->world.entities,
	hovered_rect, frame_arena);

    if (!list_is_empty(&hovered_entities)) {
        game_state->debug_state.hovered_entity = list_head(&hovered_entities)->id;
    } else {
        game_state->debug_state.hovered_entity = NULL_ENTITY_ID;
    }
}

void game_update_and_render(GameState *game_state, PlatformCode platform_code, RenderBatchList *rbs,
    FrameData frame_data, GameMemory *game_memory)
{
    debug_update(game_state, frame_data, &game_memory->temporary_memory);

    game_update(game_state, frame_data.input, frame_data.dt, &game_memory->temporary_memory);
    game_render(game_state, rbs, frame_data, &game_memory->temporary_memory);

    if (input_is_key_pressed(frame_data.input, KEY_T)) {
        game_state->debug_state.debug_menu_active = !game_state->debug_state.debug_menu_active;
    }

    if (game_state->debug_state.debug_menu_active) {
        ui_core_begin_frame(&game_state->ui);

        game_update_and_render_ui(&game_state->ui, game_state, game_memory, frame_data);

        Matrix4 proj = mat4_orthographic(frame_data.window_size, Y_IS_DOWN);
        RenderBatch *ui_rb = rb_list_push_new(rbs, proj, Y_IS_DOWN, &game_memory->temporary_memory);
        ui_core_end_frame(&game_state->ui, frame_data, ui_rb, &game_state->asset_list, platform_code);
    }
}

void game_initialize(GameState *game_state, GameMemory *game_memory)
{
    world_initialize(&game_state->world, &game_state->asset_list, &game_memory->permanent_memory);

    game_state->sb = str_builder_allocate(32, la_allocator(&game_memory->permanent_memory));
    game_state->debug_state.average_fps = 60.0f;

    magic_initialize(&game_state->asset_list);

    UIStyle default_ui_style = {
        .font = game_state->asset_list.default_font
    };

    ui_core_initialize(&game_state->ui, default_ui_style, &game_memory->permanent_memory);
}
