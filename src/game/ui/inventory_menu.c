#include "inventory_menu.h"

#include "base/rectangle.h"
#include "base/utils.h"
#include "components/inventory.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "game.h"
#include "platform/input.h"
#include "renderer/frontend/render_batch.h"

/* NOTE:
 * Having a grid inventory with draggable items requires some work-arounds due
 * to the UI being immediate mode. This is currently done by making a inventory
 * container widget and registering a hook that is called when it's rendered. In
 * this callback, we render the inventory, however since the hook is called
 * AFTER the UI has started rendering, we can't call any UI functions in it as
 * the frame is over as far as the UI is concerned.
 *
 * In the hook, we also save the dimensions of the inventory grid rectangle,
 * which isn't known until the UI has finished laying out widgets. This
 * rectangle is then used by anyone from the outside who wants to interact with
 * the inventory, for example by dragging and dropping items to/from it.
 *
 * We also store a variable that keeps track of whether the inventory widget
 * rectangle is known. Make sure to not interact with the inventory from the
 * outside unless this variable is true.
 */

#define VALID_ITEM_BG_COLOR (RGBA32){0, 0.3f, 1, 1.0f}
#define INVALID_ITEM_BG_COLOR (RGBA32){1, 0.3f, 0, 1.0f}

typedef struct {
    World *world;
    InventoryMenu *inventory_menu;
    Inventory *inventory;
    const FrameData *frame_data;
} InventoryHookContext;

typedef enum {
    GRID_COORD_TRUNCATE,
    GRID_COORD_ROUND_TO_NEAREST,
} GridCoordRounding;

static inline Vector2i screen_to_inventory_grid_coords(
    Vector2 position, Rectangle inventory_grid_rect, GridCoordRounding rounding)
{
    Vector2 screen_offset = v2_sub(position, inventory_grid_rect.position);
    Vector2 floating_grid_pos = v2_div_s(screen_offset, INVENTORY_GRID_UI_CELL_SIZE);

    if (rounding == GRID_COORD_ROUND_TO_NEAREST) {
        floating_grid_pos.x = roundf(floating_grid_pos.x);
        floating_grid_pos.y = roundf(floating_grid_pos.y);
    }

    Vector2i result = v2_to_v2i(floating_grid_pos);

    return result;
}

static inline Vector2 inventory_grid_to_screen_vector(Vector2i grid_pos)
{
    Vector2 result = {(f32)grid_pos.x * INVENTORY_GRID_UI_CELL_SIZE,
        (f32)grid_pos.y * INVENTORY_GRID_UI_CELL_SIZE};

    return result;
}

static inline Vector2 inventory_grid_to_screen_coords(
    Vector2i position, Rectangle inventory_grid_rect)
{
    Vector2 result = inventory_grid_to_screen_vector(position);
    result = v2_add(result, inventory_grid_rect.position);

    return result;
}

static Vector2i get_cursor_item_grid_coords(
    InventoryMenu *inv_menu, InventoryStorable *item, Vector2 mouse_pos)
{
    Vector2 top_left = v2_sub(
        mouse_pos, v2_div_s(inventory_grid_to_screen_vector(item->inventory_grid_size), 2.0f));
    Vector2i result = screen_to_inventory_grid_coords(
        top_left, inv_menu->inventory_grid_rect, GRID_COORD_ROUND_TO_NEAREST);

    return result;
}

static void put_item_on_cursor(Game *game, Entity *item_entity)
{
    InventoryStorable *item = es_get_component(item_entity, InventoryStorable);
    ensure_valid_item_grid_size(item);

    game->game_ui.inventory_menu.item_on_cursor =
        es_get_id_of_entity(&game->world.entity_system, item_entity);
}

void pick_up_item_from_world_and_put_on_cursor(Game *game, Entity *item_entity)
{
    put_item_on_cursor(game, item_entity);
    world_make_entity_non_spatial(&game->world, item_entity);
}

static void render_inventory_grid_overlay(
    InventoryHookContext *context, RenderBatch *rb, LinearArena *scratch)
{
    f32 grid_thickness = 2.0f;
    RGBA32 grid_color = rgba32_mono(0.5f, 1.0f);

    ASSERT(context->inventory_menu->can_interact_with_inventory);

    Rectangle grid_dims = context->inventory_menu->inventory_grid_rect;

    for (ssize row = 1; row < INVENTORY_GRID_CELL_COUNTS.y; ++row) {
        f32 y_offset = (f32)row * INVENTORY_GRID_UI_CELL_SIZE;
        Vector2 start = v2_add(grid_dims.position, v2(0.0f, y_offset));
        Vector2 end = v2_add(start, v2(grid_dims.size.x, 0));

        draw_line(rb, scratch, start, end, grid_color, grid_thickness,
            shader_handle(SHAPE_SHADER), 0);
    }

    for (ssize col = 1; col < INVENTORY_GRID_CELL_COUNTS.x; ++col) {
        f32 x_offset = (f32)col * INVENTORY_GRID_UI_CELL_SIZE;
        Vector2 start = v2_add(grid_dims.position, v2(x_offset, 0.0f));
        Vector2 end = v2_add(start, v2(0.0f, grid_dims.size.y));

        draw_line(rb, scratch, start, end, grid_color, grid_thickness,
            shader_handle(SHAPE_SHADER), 0);
    }
}

static void render_inventory_items(
    InventoryHookContext *context, RenderBatch *rb, LinearArena *scratch)
{
    EntitySystem *es = &context->world->entity_system;
    EntityID curr_item = context->inventory->first_item_in_inventory;

    while (!entity_id_is_null(curr_item)) {
        Entity *item_entity = es_get_entity(es, curr_item);
        InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

        Vector2 item_screen_pos = inventory_grid_to_screen_coords(
            item->inventory_grid_position, context->inventory_menu->inventory_grid_rect);
        Vector2 item_px_size = inventory_grid_to_screen_vector(item->inventory_grid_size);
        Rectangle item_rect = {item_screen_pos, item_px_size};

        draw_rectangle(
            rb, scratch, item_rect, VALID_ITEM_BG_COLOR, shader_handle(SHAPE_SHADER), 0);

        SpriteComponent *sprite = es_get_component(item_entity, SpriteComponent);
        draw_sprite(rb, scratch, sprite->sprite.texture, item_rect,
            zero_struct(SpriteModifiers), shader_handle(TEXTURE_SHADER), 10);

        curr_item = item->next_item_in_inventory;
    }
}

static void inventory_grid_hook(
    void *user_data, RenderBatch *rb, Widget *parent, LinearArena *frame_arena)
{
    InventoryHookContext *context = user_data;
    InventoryMenu *menu = context->inventory_menu;

    Rectangle widget_rect = {parent->final_position, parent->final_size};

    if (menu->can_interact_with_inventory
        && !rect_eq(widget_rect, menu->inventory_grid_rect)) {
        // Grid rectangle was set to known but the rectangle  has changed since last frame,
        // so set it to unknown  and wait until we once again know its dimensions
        ASSERT(0);
        menu->can_interact_with_inventory = false;
    } else {
        menu->can_interact_with_inventory = true;
    }

    menu->inventory_grid_rect = widget_rect;

    render_inventory_grid_overlay(context, rb, frame_arena);
    render_inventory_items(context, rb, frame_arena);
}

static void handle_inventory_dragging(Game *game, Inventory *inventory, Vector2 mouse_pos)
{
    ASSERT(entity_id_is_null(game->game_ui.inventory_menu.item_on_cursor));

    InventoryMenu *inv_menu = &game->game_ui.inventory_menu;
    EntitySystem *es = &game->world.entity_system;

    Vector2i mouse_grid_coords = screen_to_inventory_grid_coords(
        mouse_pos, inv_menu->inventory_grid_rect, GRID_COORD_TRUNCATE);

    EntityID hovered_item_id =
        try_get_inventory_item_at_position(es, inventory, mouse_grid_coords);

    if (!entity_id_is_null(hovered_item_id)) {
        Entity *grabbed_entity = es_get_entity(es, hovered_item_id);
        InventoryStorable *grabbed_item = es_get_component(grabbed_entity, InventoryStorable);

        put_item_on_cursor(game, grabbed_entity);
        remove_item_from_inventory(es, inventory, grabbed_item);
    }
}

static void handle_inventory_dropping(Game *game, Inventory *inventory, Vector2 mouse_pos)
{
    ASSERT(!entity_id_is_null(game->game_ui.inventory_menu.item_on_cursor));

    EntitySystem *es = &game->world.entity_system;
    Entity *cursor_item_entity =
        es_get_entity(es, game->game_ui.inventory_menu.item_on_cursor);
    InventoryStorable *cursor_item = es_get_component(cursor_item_entity, InventoryStorable);
    InventoryMenu *inv_menu = &game->game_ui.inventory_menu;

    Vector2i grid_coords = get_cursor_item_grid_coords(inv_menu, cursor_item, mouse_pos);
    b32 in_bounds =
        item_is_in_bounds_of_inventory_grid(grid_coords, cursor_item->inventory_grid_size);

    if (in_bounds) {
        InventoryInsertion insertion =
            try_place_or_exchange_inventory_item(es, inventory, cursor_item, grid_coords);

        if (insertion.ok) {
            game->game_ui.inventory_menu.item_on_cursor = insertion.exchanged_item;
        }
    }
}

static void handle_inventory_drag_and_drop(
    Game *game, Inventory *inventory, const FrameData *frame_data)
{
    ASSERT(game->game_ui.inventory_menu.can_interact_with_inventory);

    InventoryMenu *inv_menu = &game->game_ui.inventory_menu;
    Vector2 mouse_pos =
        input_get_mouse_pos(&frame_data->input, Y_IS_DOWN, frame_data->window_size);

    if (rect_contains_point(inv_menu->inventory_grid_rect, mouse_pos)
        && input_is_key_pressed(&frame_data->input, MOUSE_LEFT)) {
        if (entity_id_is_null(game->game_ui.inventory_menu.item_on_cursor)) {
            handle_inventory_dragging(game, inventory, mouse_pos);
        } else {
            handle_inventory_dropping(game, inventory, mouse_pos);
        }
    }
}

void inventory_menu(Game *game, const FrameData *frame_data, LinearArena *scratch)
{
    UIState *ui = &game->game_ui.backend_state;

    if (!game->game_ui.inventory_menu.active) {
        game->game_ui.inventory_menu.can_interact_with_inventory = false;
    } else {
        Entity *player = world_get_player_entity(&game->world);
        Inventory *inventory = es_get_component(player, Inventory);

        ui_begin_menu(
            ui, V2_ZERO, str("inventory_container"), UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);
        {
            ui_text(ui, str("Inventory"));

            ui_begin_menu(ui, INVENTORY_GRID_UI_SIZE, str("inventory_grid"),
                UI_SIZE_KIND_ABSOLUTE, 8.0f);
            {
                InventoryHookContext *context =
                    la_allocate_item(scratch, InventoryHookContext);
                context->world = &game->world;
                context->frame_data = frame_data;
                context->inventory = inventory;
                context->inventory_menu = &game->game_ui.inventory_menu;

                ui_push_render_hook(ui, inventory_grid_hook, context);

                if (game->game_ui.inventory_menu.can_interact_with_inventory) {
                    handle_inventory_drag_and_drop(game, inventory, frame_data);
                }
            }
            ui_pop_menu(ui);
        }
        ui_pop_menu(ui);
    }
}

static void render_cursor_item_background(InventoryMenu *inv_menu, Inventory *inventory,
    InventoryStorable *item, Vector2 mouse_pos, EntitySystem *es, RenderBatch *rb,
    LinearArena *arena)
{
    Vector2i item_grid_pos = get_cursor_item_grid_coords(inv_menu, item, mouse_pos);
    Vector2i item_grid_size = item->inventory_grid_size;

    Vector2i clamped_item_grid_pos = {
        MAX(item_grid_pos.x, 0),
        MAX(item_grid_pos.y, 0),
    };

    Vector2i clamp_pos_diff = v2i_sub(clamped_item_grid_pos, item_grid_pos);

    Vector2i clamped_item_grid_size = {
        MIN(item_grid_size.x, INVENTORY_GRID_CELL_COUNTS.x - item_grid_pos.x)
            - clamp_pos_diff.x,
        MIN(item_grid_size.y, INVENTORY_GRID_CELL_COUNTS.y - item_grid_pos.y)
            - clamp_pos_diff.y,
    };

    b32 item_is_inside_inventory =
        (clamped_item_grid_size.x > 0) && (clamped_item_grid_size.y > 0);

    if (item_is_inside_inventory) {
        b32 can_place_item =
            can_place_item_in_inventory_at(es, inventory, item, item_grid_pos);

        RGBA32 bg_color = {0};
        if (can_place_item) {
            bg_color = VALID_ITEM_BG_COLOR;
        } else {
            bg_color = INVALID_ITEM_BG_COLOR;
            ;
        }

        Vector2 bg_screen_size = inventory_grid_to_screen_vector(clamped_item_grid_size);
        Vector2 bg_screen_pos = inventory_grid_to_screen_coords(
            clamped_item_grid_pos, inv_menu->inventory_grid_rect);
        Rectangle bg_rect = {bg_screen_pos, bg_screen_size};

        draw_rectangle(rb, arena, bg_rect, bg_color, shader_handle(SHAPE_SHADER), 0);
    }
}

void render_item_on_cursor(Game *game, RenderBatch *rb, Vector2 mouse_pos, LinearArena *arena)
{
    GameUIState *ui = &game->game_ui;

    if (entity_id_is_null(ui->inventory_menu.item_on_cursor)) {
        return;
    }

    Entity *player = world_get_player_entity(&game->world);
    Inventory *inventory = es_get_component(player, Inventory);

    EntitySystem *es = &game->world.entity_system;
    Entity *item_entity = es_get_entity(es, ui->inventory_menu.item_on_cursor);
    InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

    item->inventory_grid_size = v2i(3, 3);

    Vector2 item_size_px = inventory_grid_to_screen_vector(item->inventory_grid_size);
    Vector2 item_top_left = v2_sub(mouse_pos, v2_div_s(item_size_px, 2.0f));

    SpriteComponent *sprite = es_try_get_component(item_entity, SpriteComponent);
    ASSERT(sprite);

    InventoryMenu *inv_menu = &game->game_ui.inventory_menu;

    if (game->game_ui.inventory_menu.can_interact_with_inventory) {
        render_cursor_item_background(inv_menu, inventory, item, mouse_pos, es, rb, arena);
    }

    if (sprite) {
        Rectangle item_sprite_rect = {item_top_left, item_size_px};

        draw_sprite(rb, arena, sprite->sprite.texture, item_sprite_rect,
            zero_struct(SpriteModifiers), shader_handle(TEXTURE_SHADER), 100);
    }
}
