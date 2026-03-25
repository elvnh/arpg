#include "inventory_menu.h"

#include "base/rectangle.h"
#include "base/utils.h"
#include "components/inventory.h"
#include "entity/entity_id.h"
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

typedef struct {
    World *world;
    InventoryMenu *inventory_menu;
    Inventory *inventory;
    const FrameData *frame_data;
} InventoryHookContext;

static Vector2i screen_to_inventory_grid_coords(
    Vector2 position, Rectangle inventory_grid_rect)
{
    Vector2 screen_offset = v2_sub(position, inventory_grid_rect.position);
    Vector2i result = v2_to_v2i(v2_div_s(screen_offset, INVENTORY_GRID_UI_CELL_SIZE));
    /* result.y = INVENTORY_GRID_CELL_COUNTS.y - result.y; */

    return result;
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

    Vector2 inv_grid_pos = context->inventory_menu->inventory_grid_rect.position;

    EntityID curr_item = context->inventory->first_item_in_inventory;

    while (!entity_id_is_null(curr_item)) {
        Entity *item_entity = es_get_entity(es, curr_item);
        InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

        Vector2 item_screen_pos = v2_add(inv_grid_pos,
            v2_mul_s(v2i_to_v2(item->inventory_grid_position), INVENTORY_GRID_UI_CELL_SIZE));

        Vector2 item_px_size =
            v2_mul_s(v2i_to_v2(item->inventory_grid_size), INVENTORY_GRID_UI_CELL_SIZE);
        Rectangle item_rect = {item_screen_pos, item_px_size};

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

static void handle_inventory_dragging(Game *game, Inventory *inventory,
    Vector2i hovered_grid_coords, const FrameData *frame_data, LinearArena *scratch)
{
    ASSERT(entity_id_is_null(game->game_ui.item_on_cursor));

    EntitySystem *es = &game->world.entity_system;

    EntityID hovered_item_id =
        try_get_inventory_item_at_position(es, inventory, hovered_grid_coords);

    if (!entity_id_is_null(hovered_item_id)) {
        game->game_ui.item_on_cursor = hovered_item_id;

        Entity *grabbed_item_entity = es_get_entity(es, hovered_item_id);
        InventoryStorable *grabbed_item =
            es_get_component(grabbed_item_entity, InventoryStorable);

        remove_item_from_inventory(es, inventory, grabbed_item);
    }
}

static void handle_inventory_dropping(Game *game, Inventory *inventory,
    Vector2i hovered_grid_coords, const FrameData *frame_data, LinearArena *scratch)
{
    (void)game;
    (void)inventory;
    (void)frame_data;
    (void)scratch;

    ASSERT(!entity_id_is_null(game->game_ui.item_on_cursor));

    EntitySystem *es = &game->world.entity_system;
    Entity *cursor_item_entity = es_get_entity(es, game->game_ui.item_on_cursor);
    InventoryStorable *cursor_item = es_get_component(cursor_item_entity, InventoryStorable);

    if (try_add_item_to_inventory_at(es, inventory, cursor_item, hovered_grid_coords)) {
        game->game_ui.item_on_cursor = NULL_ENTITY_ID;
    }
}

static void handle_inventory_drag_and_drop(
    Game *game, Inventory *inventory, const FrameData *frame_data, LinearArena *scratch)
{
    ASSERT(game->game_ui.inventory_menu.can_interact_with_inventory);

    InventoryMenu *inv_menu = &game->game_ui.inventory_menu;
    Vector2 mouse_pos =
        input_get_mouse_pos(&frame_data->input, Y_IS_DOWN, frame_data->window_size);

    if (rect_contains_point(inv_menu->inventory_grid_rect, mouse_pos)) {
        Vector2i grid_coords =
            screen_to_inventory_grid_coords(mouse_pos, inv_menu->inventory_grid_rect);

        if (input_is_key_pressed(&frame_data->input, MOUSE_LEFT)) {
            if (entity_id_is_null(game->game_ui.item_on_cursor)) {
                handle_inventory_dragging(game, inventory, grid_coords, frame_data, scratch);
            } else {
                handle_inventory_dropping(game, inventory, grid_coords, frame_data, scratch);
            }
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
                    handle_inventory_drag_and_drop(game, inventory, frame_data, scratch);
                }
            }
            ui_pop_menu(ui);
        }
        ui_pop_menu(ui);
    }
}
