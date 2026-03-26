#include "inventory_menu.h"

#include "base/rectangle.h"
#include "base/utils.h"
#include "components/inventory.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "game.h"
#include "platform/input.h"
#include "renderer/frontend/render_batch.h"
#include "world/world.h"

/* NOTE:
 *
 * The fact that the game's UI is immediate mode means we can't know the size of
 * the inventory grid before it is drawn for at least one frame. Currently this
 * is solved by creating a hook that gets called after the inventory grid is
 * drawn, and holding on to the widget rectangle, and setting a flag that
 * indicates that the size of the inventory grid is known.
 *
 * The outside world should not interact with the inventory unless this flag is
 * set.
 */

#define VALID_ITEM_BG_COLOR (RGBA32){0, 0.3f, 1, 1.0f}
#define INVALID_ITEM_BG_COLOR (RGBA32){1, 0.3f, 0, 1.0f}

typedef struct {
    World *world;
    InventoryMenu *inventory_menu;
    Inventory *inventory;
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

static void put_item_on_cursor(InventoryMenu *inv_menu, EntitySystem *es, Entity *item_entity)
{
    InventoryStorable *item = es_get_component(item_entity, InventoryStorable);
    ensure_valid_item_grid_size(item);

    inv_menu->item_on_cursor = es_get_id_of_entity(es, item_entity);
}

void pick_up_item_from_world_and_put_on_cursor(
    InventoryMenu *inv_menu, World *world, Entity *item_entity)
{
    put_item_on_cursor(inv_menu, &world->entity_system, item_entity);
    world_make_entity_non_spatial(world, item_entity);
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

/* NOTE: This gets called after the inventory gets rendered. Read the comment at
 * the top of the file for an explanation of why this is necessary.
 *
 * The fact that this hook is called after the UI is finished laying out means
 * we can't call any UI functions in it since the frame is over as far as the UI
 * is concerned.
 */
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

    // TODO: should these really be called from inside the hook?
    render_inventory_grid_overlay(context, rb, frame_arena);
    render_inventory_items(context, rb, frame_arena);
}

static void handle_inventory_dragging(
    InventoryMenu *inv_menu, EntitySystem *es, Inventory *inventory, Vector2 mouse_pos)
{
    ASSERT(entity_id_is_null(inv_menu->item_on_cursor));

    Vector2i mouse_grid_coords = screen_to_inventory_grid_coords(
        mouse_pos, inv_menu->inventory_grid_rect, GRID_COORD_TRUNCATE);

    EntityID hovered_item_id =
        try_get_inventory_item_at_position(es, inventory, mouse_grid_coords);

    if (!entity_id_is_null(hovered_item_id)) {
        Entity *grabbed_entity = es_get_entity(es, hovered_item_id);
        InventoryStorable *grabbed_item = es_get_component(grabbed_entity, InventoryStorable);

        put_item_on_cursor(inv_menu, es, grabbed_entity);
        remove_item_from_inventory(es, inventory, grabbed_item);
    }
}

static void drop_cursor_item_into_world(
    InventoryMenu *inv_menu, EntitySystem *es, Vector2 player_pos)
{
    Entity *cursor_entity = es_get_entity(es, inv_menu->item_on_cursor);
    world_drop_item_from_position(player_pos, cursor_entity);

    inv_menu->item_on_cursor = NULL_ENTITY_ID;
}

// TODO: UI shouldn't directly alter world state, instead create some sort
// of command buffer that gets executed when world is updated
static void handle_inventory_dropping(InventoryMenu *inv_menu, EntitySystem *es,
    Vector2 player_pos, Inventory *inventory, Vector2 mouse_pos)
{
    Entity *cursor_item_entity = es_get_entity(es, inv_menu->item_on_cursor);
    InventoryStorable *cursor_item = es_get_component(cursor_item_entity, InventoryStorable);

    if (inv_menu->can_interact_with_inventory) {
        ASSERT(inv_menu->active);

        Vector2i item_grid_coords =
            get_cursor_item_grid_coords(inv_menu, cursor_item, mouse_pos);

        b32 in_bounds = item_is_in_bounds_of_inventory_grid(
            item_grid_coords, cursor_item->inventory_grid_size);

        if (in_bounds) {
            InventoryInsertion insertion = try_place_or_exchange_inventory_item(
                es, inventory, cursor_item, item_grid_coords);

            if (insertion.ok) {
                inv_menu->item_on_cursor = insertion.exchanged_item;
            }
        } else if (!cell_is_in_bounds_of_inventory_grid(item_grid_coords)) {
            // If the entire item is outside the inventory, we drop it
            drop_cursor_item_into_world(inv_menu, es, player_pos);
        }
    } else {
        // If inventory isn't open, always drop item
        drop_cursor_item_into_world(inv_menu, es, player_pos);
    }
}

static void handle_inventory_drag_and_drop(InventoryMenu *inv_menu, Vector2 player_pos,
    Inventory *inventory, World *world, Vector2 mouse_pos)
{
    ASSERT(inv_menu->can_interact_with_inventory);

    if (entity_id_is_null(inv_menu->item_on_cursor)
        && rect_contains_point(inv_menu->inventory_grid_rect, mouse_pos)) {
        handle_inventory_dragging(inv_menu, &world->entity_system, inventory, mouse_pos);
    } else {
        handle_inventory_dropping(
            inv_menu, &world->entity_system, player_pos, inventory, mouse_pos);
    }
}

void inventory_menu(UIState *ui, InventoryMenu *inv_menu, World *world,
    const FrameData *frame_data, LinearArena *scratch)
{
    Entity *player = world_get_player_entity(world);
    Inventory *inventory = es_get_component(player, Inventory);
    PhysicsComponent *player_physics = es_get_component(player, PhysicsComponent);

    Vector2 mouse_pos =
        input_get_mouse_pos(&frame_data->input, Y_IS_DOWN, frame_data->window_size);
    b32 mouse_pressed = input_is_key_pressed(&frame_data->input, MOUSE_LEFT);

    if (!inv_menu->active) {
        inv_menu->can_interact_with_inventory = false;

        if (mouse_pressed) {
            drop_cursor_item_into_world(
                inv_menu, &world->entity_system, player_physics->position);
        }
    } else {
        ui_begin_menu(
            ui, V2_ZERO, str("inventory_container"), UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);
        {
            ui_text(ui, str("Inventory"));

            ui_begin_menu(ui, INVENTORY_GRID_UI_SIZE, str("inventory_grid"),
                UI_SIZE_KIND_ABSOLUTE, 8.0f);
            {
                InventoryHookContext *context =
                    la_allocate_item(scratch, InventoryHookContext);
                context->world = world;
                context->inventory = inventory;
                context->inventory_menu = inv_menu;

                ui_push_render_hook(ui, inventory_grid_hook, context);

                if (inv_menu->can_interact_with_inventory && mouse_pressed) {
                    handle_inventory_drag_and_drop(
                        inv_menu, player_physics->position, inventory, world, mouse_pos);
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

void render_item_on_cursor(InventoryMenu *inv_menu, World *world, RenderBatch *rb,
    Vector2 mouse_pos, LinearArena *arena)
{
    if (entity_id_is_null(inv_menu->item_on_cursor)) {
        return;
    }

    Entity *player = world_get_player_entity(world);
    Inventory *inventory = es_get_component(player, Inventory);

    Entity *item_entity = es_get_entity(&world->entity_system, inv_menu->item_on_cursor);
    InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

    // TODO: remove
    item->inventory_grid_size = v2i(3, 3);

    Vector2 item_size_px = inventory_grid_to_screen_vector(item->inventory_grid_size);
    Vector2 item_top_left = v2_sub(mouse_pos, v2_div_s(item_size_px, 2.0f));

    SpriteComponent *sprite = es_try_get_component(item_entity, SpriteComponent);
    ASSERT(sprite);

    if (inv_menu->can_interact_with_inventory) {
        render_cursor_item_background(
            inv_menu, inventory, item, mouse_pos, &world->entity_system, rb, arena);
    }

    if (sprite) {
        Rectangle item_sprite_rect = {item_top_left, item_size_px};

        draw_sprite(rb, arena, sprite->sprite.texture, item_sprite_rect,
            zero_struct(SpriteModifiers), shader_handle(TEXTURE_SHADER), 100);
    }
}
