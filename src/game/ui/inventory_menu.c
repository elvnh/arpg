#include "inventory_menu.h"

#include "base/rectangle.h"
#include "base/utils.h"
#include "base/vector.h"
#include "command.h"
#include "components/equipment.h"
#include "components/inventory.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "game.h"
#include "platform/input.h"
#include "platform/input_event.h"
#include "renderer/frontend/render_batch.h"
#include "ui/ui_core.h"
#include "ui/widget.h"
#include "world/world.h"

/* NOTE:
 *
 * The fact that the game's UI is immediate mode means we can't know the size of
 * the inventory grid before it is drawn for at least one frame. Currently this
 * is solved by creating a hook that gets called after the inventory grid is
 * drawn, and holding on to the widget rectangle, and setting a flag that
 * indicates that the size of the inventory grid is known.
 *
 * This means the inventory can't be interacted with until it has been visible
 * for one frame. The inventory is drawn from inside the hook though, so this
 * shouldn't cause any visual issues, and the 1 frame delay for interacting with
 * items in the inventory shouldn't be a problem.
 */

#define VALID_ITEM_BG_COLOR (RGBA32){0, 0.3f, 1, 1.0f}
#define INVALID_ITEM_BG_COLOR (RGBA32){1, 0.3f, 0, 1.0f}

/* NOTE:
 *
 * This a hacky way of making the widgets appear in the correct order.
 *
 * TODO: fix this properly
 */
enum {
    INVENTORY_GRID_LAYER,
    INVENTORY_ITEM_BACKGROUND_LAYER,
    INVENTORY_ITEM_SPRITE_LAYER,
    CURSOR_ITEM_BACKGROUND_LAYER,
    CURSOR_ITEM_SPRITE_LAYER,
};

typedef struct {
    UIState *ui;
    World *world;
    InventoryMenu *inventory_menu;
    Inventory *inventory;
    Equipment *equipment;
    Vector2 mouse_pos;
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
            shader_handle(SHAPE_SHADER), (RenderLayer)INVENTORY_GRID_LAYER);
    }

    for (ssize col = 1; col < INVENTORY_GRID_CELL_COUNTS.x; ++col) {
        f32 x_offset = (f32)col * INVENTORY_GRID_UI_CELL_SIZE;
        Vector2 start = v2_add(grid_dims.position, v2(x_offset, 0.0f));
        Vector2 end = v2_add(start, v2(0.0f, grid_dims.size.y));

        draw_line(rb, scratch, start, end, grid_color, grid_thickness,
            shader_handle(SHAPE_SHADER), (RenderLayer)INVENTORY_GRID_LAYER);
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

        draw_rectangle(rb, scratch, item_rect, VALID_ITEM_BG_COLOR,
            shader_handle(SHAPE_SHADER), (RenderLayer)INVENTORY_ITEM_BACKGROUND_LAYER);

        SpriteComponent *sprite = es_get_component(item_entity, SpriteComponent);

        // TODO: a layer higher that the background rectangle is required here,
        // otherwise the sprite appears behing the rectangle. Figure out why,
        // since this shouldn't be the case as a stable sort is used for sorting
        // render commands.
        draw_sprite(rb, scratch, sprite->sprite.texture, item_rect,
            zero_struct(SpriteModifiers), shader_handle(TEXTURE_SHADER),
            (RenderLayer)INVENTORY_ITEM_SPRITE_LAYER);

        curr_item = item->next_item_in_inventory;
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
        InventoryInsertion insertion =
            can_place_or_exchange_inventory_item_at(es, inventory, item, item_grid_pos);
        b32 can_place_item = insertion.ok && entity_id_is_null(insertion.exchanged_item);

        RGBA32 bg_color = {0};
        if (can_place_item) {
            bg_color = VALID_ITEM_BG_COLOR;
        } else {
            bg_color = INVALID_ITEM_BG_COLOR;
        }

        Vector2 bg_screen_size = inventory_grid_to_screen_vector(clamped_item_grid_size);
        Vector2 bg_screen_pos = inventory_grid_to_screen_coords(
            clamped_item_grid_pos, inv_menu->inventory_grid_rect);
        Rectangle bg_rect = {bg_screen_pos, bg_screen_size};

        draw_rectangle(rb, arena, bg_rect, bg_color, shader_handle(SHAPE_SHADER),
            (RenderLayer)CURSOR_ITEM_BACKGROUND_LAYER);
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
            zero_struct(SpriteModifiers), shader_handle(TEXTURE_SHADER),
            (RenderLayer)CURSOR_ITEM_SPRITE_LAYER);
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
        // TODO: this isn't necessary?
        ASSERT(0);
        menu->can_interact_with_inventory = false;
    } else {
        menu->can_interact_with_inventory = true;
    }

    menu->inventory_grid_rect = widget_rect;

    // TODO: should these really be called from inside the hook?
    render_inventory_grid_overlay(context, rb, frame_arena);
    render_inventory_items(context, rb, frame_arena);

    // TODO: don't call this from within hook
    render_item_on_cursor(menu, context->world, rb, context->mouse_pos, frame_arena);
}

static void render_equipment_slot_containers(UIState *ui, InventoryMenu *inv_menu,
    World *world, Equipment *equipment, RenderBatch *rb, Vector2 mouse_pos,
    LinearArena *frame_arena)
{
    UIStyle style = ui_get_current_style(ui);

    RGBA32 slot_color = style.background_shadow_color;

    for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
        Rectangle rect = inv_menu->equipment_slot_rects[slot];

        draw_rectangle(rb, frame_arena, rect, slot_color, shader_handle(SHAPE_SHADER),
            (RenderLayer)INVENTORY_ITEM_BACKGROUND_LAYER);

        Entity *equipped_entity =
            get_equipped_item_in_slot(&world->entity_system, equipment, slot);
        SpriteComponent *sprite = es_try_get_component(equipped_entity, SpriteComponent);

        if (sprite) {
            draw_sprite(rb, frame_arena, sprite->sprite.texture, rect,
                zero_struct(SpriteModifiers), shader_handle(TEXTURE_SHADER),
                (RenderLayer)INVENTORY_ITEM_SPRITE_LAYER);
        }

        draw_text(rb, frame_arena, equipment_slot_to_string(slot),
            v2_add(rect.position, v2_div_s(rect.size, 2)), RGBA32_WHITE, 14,
            shader_handle(TEXTURE_SHADER), font_handle(DEFAULT_FONT),
            (RenderLayer)(INVENTORY_ITEM_SPRITE_LAYER + 1));
    }
}

static void set_equipment_slot_rectangles(InventoryMenu *inv_menu, Vector2 base,
    Vector2 menu_size, f32 pad)
{
    // TODO: this is all very bad, handle the layout in a better way
    f32 base_slot_size = 80.0f;

    // clang-format off
    Vector2 slot_sizes[] = {
        [EQUIP_SLOT_HEAD] = {base_slot_size, base_slot_size},
        [EQUIP_SLOT_GLOVES] = {base_slot_size, base_slot_size},
        [EQUIP_SLOT_WEAPON] = {base_slot_size, base_slot_size * 2.0f},
        [EQUIP_SLOT_BODY] = {base_slot_size, base_slot_size * 2.0f},
        [EQUIP_SLOT_LEGS] = {base_slot_size, base_slot_size * 2.0f},
        [EQUIP_SLOT_FEET] = {base_slot_size, base_slot_size},
        [EQUIP_SLOT_NECK] = {base_slot_size / 2.0f, base_slot_size / 2.0f},
        [EQUIP_SLOT_LEFT_FINGER] = {base_slot_size / 2.0f, base_slot_size / 2.0f},
        [EQUIP_SLOT_RIGHT_FINGER] = {base_slot_size / 2.0f, base_slot_size / 2.0f},
    };
    // clang-format on

    Vector2 slot_locations[EQUIP_SLOT_COUNT] = {0};

    f32 center_x = menu_size.x / 2.0f + pad - base_slot_size / 2.0f;

    slot_locations[EQUIP_SLOT_HEAD] = v2(center_x, pad);

    slot_locations[EQUIP_SLOT_BODY] =
        v2(center_x, slot_locations[EQUIP_SLOT_HEAD].y + slot_sizes[EQUIP_SLOT_HEAD].y + pad);

    slot_locations[EQUIP_SLOT_NECK] =
        v2(slot_locations[EQUIP_SLOT_BODY].x + slot_sizes[EQUIP_SLOT_BODY].x + pad,
            slot_locations[EQUIP_SLOT_BODY].y + pad);

    slot_locations[EQUIP_SLOT_LEFT_FINGER] = v2(slot_locations[EQUIP_SLOT_NECK].x,
        slot_locations[EQUIP_SLOT_NECK].y + slot_sizes[EQUIP_SLOT_NECK].y + pad);

    slot_locations[EQUIP_SLOT_RIGHT_FINGER] = v2(
        slot_locations[EQUIP_SLOT_LEFT_FINGER].x + slot_sizes[EQUIP_SLOT_LEFT_FINGER].x + pad,
        slot_locations[EQUIP_SLOT_LEFT_FINGER].y);

    slot_locations[EQUIP_SLOT_LEGS] =
        v2(center_x, slot_locations[EQUIP_SLOT_BODY].y + slot_sizes[EQUIP_SLOT_BODY].y + pad);

    slot_locations[EQUIP_SLOT_FEET] =
        v2(slot_locations[EQUIP_SLOT_LEGS].x + slot_sizes[EQUIP_SLOT_LEGS].x + pad,
            slot_locations[EQUIP_SLOT_LEGS].y + slot_sizes[EQUIP_SLOT_LEGS].y / 2.0f);

    slot_locations[EQUIP_SLOT_WEAPON] = v2(center_x - slot_sizes[EQUIP_SLOT_WEAPON].x - pad,
        slot_locations[EQUIP_SLOT_BODY].y);

    slot_locations[EQUIP_SLOT_GLOVES] =
        v2(slot_locations[EQUIP_SLOT_LEGS].x - slot_sizes[EQUIP_SLOT_GLOVES].x - pad,
            slot_locations[EQUIP_SLOT_FEET].y);

    for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
        ASSERT(!v2_eq(slot_sizes[slot], V2_ZERO))

        inv_menu->equipment_slot_rects[slot].position =
            v2_add(base, v2_add(slot_locations[slot], v2_from_scalar(0.0f)));

        inv_menu->equipment_slot_rects[slot].size = slot_sizes[slot];
    }
}

static void equipment_grid_hook(void *user_data, RenderBatch *rb, Widget *parent,
    LinearArena *frame_arena)
{
    InventoryHookContext *context = user_data;
    InventoryMenu *inv_menu = context->inventory_menu;

    f32 padding = parent->child_padding;
    set_equipment_slot_rectangles(inv_menu, parent->final_position, parent->final_size,
        padding);

    render_equipment_slot_containers(context->ui, inv_menu, context->world, context->equipment,
        rb, context->mouse_pos, frame_arena);
}

static String get_item_name_widget_text(Entity *item_entity, LinearArena *arena)
{
    String result = {0};

    NameComponent *name = es_try_get_component(item_entity, NameComponent);

    if (name) {
        // Append the item ID to ensure that there are no widget ID collisions
        result = format(arena, "%.*s##%d,%d", name->length, name->data, item_entity->id.index,
            item_entity->id.generation);
    } else {
        result = str("(unnamed item)");
    }

    return result;
}

static void item_hover_menu(UIState *ui, Entity *item, Vector2 mouse_position,
    LinearArena *arena)
{
    ASSERT(es_has_component(item, InventoryStorable));

    ui_begin_mouse_menu(ui, mouse_position);
    {
        ui_text(ui, get_item_name_widget_text(item, arena));

        ui_spacing(ui, 12);

        ItemModifiers *mods = es_try_get_component(item, ItemModifiers);

        if (mods) {
            for (ssize i = 0; i < mods->modifier_count; ++i) {
                Modifier mod = mods->modifiers[i];
                String mod_string = modifier_to_string(mod, arena);

                ui_text(ui, mod_string);
                ui_spacing(ui, 12);
            }
        }
    }
    ui_end_mouse_menu(ui);
}

static void handle_inventory_drag_and_drop(InventoryMenu *inv_menu, Inventory *inventory,
    World *world, Vector2 mouse_pos, CommandQueue *commands, InputEvents *input,
    LinearArena *arena)
{
    ASSERT(inv_menu->can_interact_with_inventory);

    Vector2i mouse_grid_pos = screen_to_inventory_grid_coords(mouse_pos,
        inv_menu->inventory_grid_rect, GRID_COORD_TRUNCATE);

    EntityID grabbed_entity_id =
        try_get_inventory_item_at_position(&world->entity_system, inventory, mouse_grid_pos);
    Entity *grabbed_entity = es_try_get_entity(&world->entity_system, grabbed_entity_id);

    EntityID moved_item = {0};
    ItemLocation source = {0};
    ItemLocation destination = {0};

    if (grabbed_entity) {
        InventoryStorable *grabbed_item = es_get_component(grabbed_entity, InventoryStorable);

        moved_item = grabbed_entity_id;
        source = item_location_inventory(grabbed_item->inventory_grid_position);

        if (check_key_down(input, KEY_LEFT_CONTROL)) {
            destination = item_location_any_equipment_slot();
        } else {
            destination = item_location_cursor();
        }
    } else {
        Entity *cursor_entity =
            es_try_get_entity(&world->entity_system, inv_menu->item_on_cursor);

        if (cursor_entity) {
            InventoryStorable *cursor_item =
                es_get_component(cursor_entity, InventoryStorable);

            Vector2i destination_grid_pos =
                get_cursor_item_grid_coords(inv_menu, cursor_item, mouse_pos);

            moved_item = inv_menu->item_on_cursor;
            source = item_location_cursor();
            destination = item_location_inventory(destination_grid_pos);
        }
    }

    Command command = move_item_command(moved_item, source, destination);
    push_command(commands, command, arena);
}

static void handle_equipment_drag_and_drop(InventoryMenu *inv_menu, Inventory *inventory,
    Equipment *equipment, World *world, Vector2 mouse_pos, CommandQueue *commands,
    InputEvents *input, LinearArena *arena)
{
    ASSERT(inv_menu->can_interact_with_inventory);

    for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
        Rectangle rect = inv_menu->equipment_slot_rects[slot];

        if (rect_contains_point(rect, mouse_pos)) {
            Command command = {0};

            if (entity_id_is_null(inv_menu->item_on_cursor)) {
                EntityID equipped =
                    get_equipped_item_id_in_slot(&world->entity_system, equipment, slot);

                ItemLocation destination = {0};

                if (check_key_down(input, KEY_LEFT_CONTROL)) {
                    destination = item_location_any_inventory_slot();
                } else {
                    destination = item_location_cursor();
                }

                command = move_item_command(equipped, item_location_equipment_slot(slot),
                    destination);
            } else {
                EntityID item = {0};
                ItemLocation source = {0};
                ItemLocation destination = {0};

                if (check_key_down(input, KEY_LEFT_CONTROL)) {
                    item =
                        get_equipped_item_id_in_slot(&world->entity_system, equipment, slot);
                    source = item_location_equipment_slot(slot);
                    destination = item_location_any_inventory_slot();
                } else {
                    item = inv_menu->item_on_cursor;
                    source = item_location_cursor();
                    destination = item_location_equipment_slot(slot);
                }

                command = move_item_command(item, source, destination);
            }

            push_command(commands, command, arena);

            break;
        }
    }
}

static void render_equipment_item_hover_menu(UIState *ui, InventoryMenu *inv_menu,
    Inventory *inventory, Equipment *equipment, World *world, Vector2 mouse_pos,
    LinearArena *arena)
{
    ASSERT(inv_menu->can_interact_with_inventory);

    for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
        Entity *equipped_item =
            get_equipped_item_in_slot(&world->entity_system, equipment, slot);

        if (equipped_item) {
            Rectangle rect = inv_menu->equipment_slot_rects[slot];
            ASSERT(rect_is_valid(rect));

            if (rect_contains_point(rect, mouse_pos)) {
                item_hover_menu(ui, equipped_item, mouse_pos, arena);
                break;
            }
        }
    }
}

static void render_inventory_item_hover_menu(UIState *ui, InventoryMenu *inv_menu,
    Inventory *inventory, World *world, Vector2 mouse_pos, LinearArena *arena)
{
    EntityID curr_item = inventory->first_item_in_inventory;

    // TODO: looping through items in inventory is getting old, figure
    // out a better way to do it
    while (!entity_id_is_null(curr_item)) {
        Entity *item_entity = es_get_entity(&world->entity_system, curr_item);
        InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

        Vector2 item_screen_pos = inventory_grid_to_screen_coords(
            item->inventory_grid_position, inv_menu->inventory_grid_rect);
        Vector2 item_px_size = inventory_grid_to_screen_vector(item->inventory_grid_size);
        Rectangle item_rect = {item_screen_pos, item_px_size};

        if (rect_contains_point(item_rect, mouse_pos)) {
            item_hover_menu(ui, item_entity, mouse_pos, arena);
            break;
        }

        curr_item = item->next_item_in_inventory;
    }
}

void inventory_menu_update(InventoryMenu *inv_menu)
{
    // TODO: this function shouldn't be needed
    if (!inv_menu->active) {
        inv_menu->can_interact_with_inventory = false;
    }
}

void inventory_menu(UIState *ui, InventoryMenu *inv_menu, World *world, InputEvents *input,
    CommandQueue *commands, LinearArena *scratch)
{
    ASSERT(inv_menu->active);

    Entity *player = world_get_player_entity(world);
    Inventory *inventory = es_get_component(player, Inventory);
    Equipment *equipment = es_get_component(player, Equipment);

    Vector2 mouse_pos = get_mouse_pos(input);

    // TODO: calculate this based on equipment slot sizes
    Vector2 equipment_menu_size = {512, 450};

    InventoryHookContext *hook_context = la_allocate_item(scratch, InventoryHookContext);
    hook_context->ui = ui;
    hook_context->world = world;
    hook_context->inventory = inventory;
    hook_context->equipment = equipment;
    hook_context->inventory_menu = inv_menu;
    hook_context->mouse_pos = mouse_pos;

    ui_begin_menu(ui, V2_ZERO, str("inv_and_eq_container"), UI_SIZE_KIND_SUM_OF_CHILDREN,
        8.0f);
    {
        ui_begin_menu(ui, equipment_menu_size, str("equipment_menu"), UI_SIZE_KIND_ABSOLUTE,
            8.0f);
        {
            ui_push_render_hook(ui, equipment_grid_hook, hook_context);

            if (inv_menu->can_interact_with_inventory) {
                render_equipment_item_hover_menu(ui, inv_menu, inventory, equipment, world,
                    mouse_pos, scratch);

                if (check_key_pressed(input, MOUSE_LEFT)) {
                    handle_equipment_drag_and_drop(inv_menu, inventory, equipment, world,
                        mouse_pos, commands, input, scratch);
                }
            }
        }
        ui_pop_menu(ui);

        ui_begin_menu(ui, V2_ZERO, str("inventory_menu"), UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);
        {
            ui_text(ui, str("Inventory"));

            ui_begin_menu(ui, INVENTORY_GRID_UI_SIZE, str("inventory_grid"),
                UI_SIZE_KIND_ABSOLUTE, 8.0f);
            {
                ui_push_render_hook(ui, inventory_grid_hook, hook_context);

                if (inv_menu->can_interact_with_inventory) {
                    render_inventory_item_hover_menu(ui, inv_menu, inventory, world, mouse_pos,
                        scratch);

                    // NOTE: we don't consume the key press here as the UI will do
                    // that if necessary
                    b32 mouse_pressed = check_key_pressed(input, MOUSE_LEFT);
                    if (mouse_pressed) {
                        handle_inventory_drag_and_drop(inv_menu, inventory, world, mouse_pos,
                            commands, input, scratch);
                    }
                }
            }
            ui_pop_menu(ui);
        }
        ui_pop_menu(ui);
    }
    ui_pop_menu(ui);
}
