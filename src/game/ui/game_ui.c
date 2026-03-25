#include "game_ui.h"

#include "base/format.h"
#include "base/string8.h"
#include "components/component.h"
#include "components/equipment.h"
#include "components/inventory.h"
#include "components/modifier.h"
#include "components/name.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "game.h"
#include "inventory_menu.h"
#include "magic.h"
#include "platform/input.h"
#include "renderer/frontend/render_batch.h"
#include "ui/ui_builder.h"
#include "ui/widget.h"
#include "world/world.h"

// TODO: clean up this file

static void spellbook_menu(GameUIState *ui_state, Game *game)
{
    UIState *ui = &ui_state->backend_state;

    ui_begin_menu(ui, V2_ZERO, str("spellbook_menu"), UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);
    {
        ui_text(ui, str("Spellbook"));

        ui_begin_list(ui, str("spellbook_list"));
        {
            Entity *player = world_get_player_entity(&game->world);
            SpellCasterComponent *spellcaster = es_get_component(player, SpellCasterComponent);

            for (ssize i = 0; i < spellcaster->spell_count; ++i) {
                String spell_name = spell_type_to_string(spellcaster->spellbook[i]);
                WidgetInteraction interaction = ui_selectable(ui, spell_name);

                // TODO: print info about spell
                if (interaction.clicked) {
                    ui_state->selected_spellbook_index = i;
                }
            }
        }
        ui_end_list(ui);
    }
    ui_pop_container(ui);
}

static String get_item_widget_text(Entity *item_entity, LinearArena *arena)
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

static void item_hover_menu(
    UIState *ui, Entity *item, Vector2 mouse_position, LinearArena *arena)
{
    ASSERT(es_has_component(item, InventoryStorable));

    ui_begin_mouse_menu(ui, mouse_position);
    {
        ui_text(ui, get_item_widget_text(item, arena));

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

static void equipment_slot_widget(GameUIState *ui_state, Game *game, Equipment *equipment,
    Inventory *inventory, EquipmentSlot slot, LinearArena *scratch, const Input *input)
{
    UIState *ui = &ui_state->backend_state;

    ui_text(ui, equipment_slot_to_string(slot));
    ui_core_same_line(ui);

    Entity *item = get_equipped_item_in_slot(&game->world.entity_system, equipment, slot);

    if (item) {
        String text = get_item_widget_text(item, scratch);
        WidgetInteraction interaction = ui_button(ui, text);

        if (interaction.clicked) {
            unequip_item_and_put_in_inventory(
                &game->world.entity_system, equipment, inventory, slot);
        } else if (interaction.hovered) {
            item_hover_menu(ui, item, input->mouse_position, scratch);
        }
    } else {
        ui_non_interactible_button(ui, str("(empty)"));
    }
}

static void equipment_menu(
    GameUIState *ui_state, Game *game, LinearArena *scratch, const FrameData *frame_data)
{
    UIState *ui = &ui_state->backend_state;
    Entity *player = world_get_player_entity(&game->world);

    ui_begin_menu(ui, V2_ZERO, str("equipment_container"), UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);
    {
        ui_text(ui, str("Equipment"));

        Equipment *eq = es_get_component(player, Equipment);
        Inventory *inv = es_get_component(player, Inventory);

        for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
            equipment_slot_widget(ui_state, game, eq, inv, slot, scratch, &frame_data->input);
        }
    }
    ui_pop_container(ui);
}

void put_item_on_cursor(GameUIState *game_ui, World *world, Entity *item_entity)
{
    InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

    // TODO: break this out into function since it's repeated
    item->inventory_grid_size.x = MAX(1, item->inventory_grid_size.x);
    item->inventory_grid_size.y = MAX(1, item->inventory_grid_size.y);

    game_ui->item_on_cursor = es_get_id_of_entity(&world->entity_system, item_entity);

    world_make_entity_non_spatial(world, item_entity);
}

#if 0

typedef struct {
    Game *game;
    const Input *input;
    Inventory *inventory;
    Vector2i window_size;
} InventoryHookData;

static void render_inventory_items(InventoryHookData *hook_data, RenderBatch *rb, Vector2 inventory_grid_position,
    LinearArena *frame_arena)
{
    GameUIState *game_ui = &hook_data->game->game_ui;
    EntitySystem *es = &hook_data->game->world.entity_system;

    EntityID curr_item = hook_data->inventory->first_item_in_inventory;

    while (!entity_id_is_null(curr_item)) {
        Entity *item_entity = es_get_entity(es, curr_item);
        InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

        Vector2 item_pos = v2_add(
            inventory_grid_position,
            v2_mul_s(v2i_to_v2(item->inventory_grid_position), INVENTORY_GRID_UI_CELL_SIZE)
        );

        Vector2 item_px_size = v2_mul_s(v2i_to_v2(item->inventory_grid_size), INVENTORY_GRID_UI_CELL_SIZE);
        Rectangle item_rect = {item_pos, item_px_size};
        SpriteComponent *sprite = es_get_component(item_entity, SpriteComponent);

        draw_sprite(rb, frame_arena, sprite->sprite.texture, item_rect, zero_struct(SpriteModifiers),
            shader_handle(TEXTURE_SHADER), 10);

        if (input_is_key_pressed(hook_data->input, KEY_L) && entity_id_is_null(game_ui->item_on_cursor)) {
            Vector2 mouse_pos = input_get_mouse_pos(hook_data->input, Y_IS_DOWN, hook_data->window_size);
            Vector2 mouse_offset = v2_sub(mouse_pos, inventory_grid_position);
            Vector2i mouse_grid_pos = v2_to_v2i(v2_div_s(mouse_pos, INVENTORY_GRID_UI_CELL_SIZE));

            EntityID grabbed_item_id = try_get_inventory_item_at_position(es, hook_data->inventory, mouse_grid_pos);

            if (!entity_id_is_null(grabbed_item_id)) {
                Entity *grabbed_item_entity = es_get_entity(es, grabbed_item_id);
                InventoryStorable *grabbed_item = es_get_component(grabbed_item_entity, InventoryStorable);

                put_item_on_cursor(game_ui, &hook_data->game->world, grabbed_item_entity);
                remove_item_from_inventory(es, hook_data->inventory, grabbed_item);
            }
        }

        curr_item = item->next_item_in_inventory;
    }
}

static void inventory_grid_hook(void *user_data, RenderBatch *rb, Widget *parent, LinearArena *frame_arena)
{
    (void)user_data;
    (void)rb;
    (void)parent;
    (void)frame_arena;

    InventoryHookData *hook_data = user_data;
    GameUIState *game_ui = &hook_data->game->game_ui;
    EntitySystem *es = &hook_data->game->world.entity_system;

    // Render grid


    render_inventory_items(hook_data, rb, parent->final_position, frame_arena);

    Rectangle grid_rect = {parent->final_position, parent->final_size};

    if (!entity_id_is_null(game_ui->item_on_cursor)) {
        // TODO: clean this up
        Entity *item_entity = es_get_entity(es, game_ui->item_on_cursor);
        InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

        item->inventory_grid_size = v2i(3, 3);

        // TODO: break out this calculation into function
        Vector2 item_size = v2_mul_s(v2i_to_v2(item->inventory_grid_size), INVENTORY_GRID_UI_CELL_SIZE);
        Vector2 mouse_pos = input_get_mouse_pos(hook_data->input, Y_IS_DOWN, hook_data->window_size);

        Rectangle item_rect = {mouse_pos, item_size};

        Vector2 mouse_offset = v2_sub(mouse_pos, parent->final_position);
        f32 mouse_truncated_x = (f32)((s32)mouse_offset.x / INVENTORY_GRID_UI_CELL_SIZE) * INVENTORY_GRID_UI_CELL_SIZE;
        f32 mouse_truncated_y = (f32)((s32)mouse_offset.y / INVENTORY_GRID_UI_CELL_SIZE) * INVENTORY_GRID_UI_CELL_SIZE;

        Vector2i item_grid_pos = {(s32)mouse_truncated_x / INVENTORY_GRID_UI_CELL_SIZE,
            (s32)mouse_truncated_y / INVENTORY_GRID_UI_CELL_SIZE};

        mouse_truncated_x += parent->final_position.x;
        mouse_truncated_y += parent->final_position.y;

        Rectangle item_grid_rect = {{mouse_truncated_x, mouse_truncated_y}, item_size};

        Rectangle overlap = rect_overlap_area(grid_rect, item_grid_rect);

        if (rect_is_valid(overlap)) {
            RGBA32 color_a = rgba32(0, 0, 1, 0.5f);
            RGBA32 color_b = rgba32(0, 1, 0, 0.5f);

            draw_rectangle(rb, frame_arena, item_rect, color_a, shader_handle(SHAPE_SHADER), 10);
            draw_rectangle(rb, frame_arena, overlap, color_b, shader_handle(SHAPE_SHADER), 10);
        }

        if (input_is_key_pressed(hook_data->input, KEY_K)) {
            b32 added = try_add_item_to_inventory_at(es, hook_data->inventory, item, item_grid_pos);

            if (added) {
                item->inventory_grid_position = item_grid_pos;

                game_ui->item_on_cursor = NULL_ENTITY_ID;
            }
        }
    }
}


static void inventory_menu_new(
    GameUIState *ui_state, Game *game, LinearArena *scratch, const FrameData *frame_data)
{
    UIState *ui = &ui_state->backend_state;

    Entity *player = world_get_player_entity(&game->world);
    Inventory *inventory = es_get_component(player, Inventory);

    ui_begin_menu(ui, V2_ZERO, str("inventory_container"), UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f); {
        ui_text(ui, str("Inventory"));

        ui_begin_menu(ui, INVENTORY_GRID_UI_SIZE, str("inventory_grid"), UI_SIZE_KIND_ABSOLUTE, 8.0f); {
            InventoryHookData *hook_data = la_allocate_item(scratch, InventoryHookData);
            hook_data->game = game;
            hook_data->input = &frame_data->input;
            hook_data->window_size = frame_data->window_size;
            hook_data->inventory = inventory;

            ui_push_render_hook(ui, inventory_grid_hook, hook_data);
        } ui_pop_menu(ui);
    } ui_pop_menu(ui);
}
#endif

void render_item_on_cursor(
    GameUIState *ui, World *world, RenderBatch *rb, Vector2 mouse_pos, LinearArena *arena)
{
    if (entity_id_is_null(ui->item_on_cursor)) {
        return;
    }

    Entity *item_entity = es_get_entity(&world->entity_system, ui->item_on_cursor);
    InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

    Vector2 item_size_px =
        v2_mul_s(v2i_to_v2(item->inventory_grid_size), INVENTORY_GRID_UI_CELL_SIZE);

    SpriteComponent *sprite = es_try_get_component(item_entity, SpriteComponent);
    ASSERT(sprite);

    if (sprite) {
        Rectangle rect = {mouse_pos, item_size_px};
        RGBA32 tint = {1, 1, 1, 0.8f};

        draw_rectangle(rb, arena, rect, rgba32(0, 1, 0, 0.5f), shader_handle(SHAPE_SHADER), 0);
        draw_colored_sprite(rb, arena, sprite->sprite.texture, rect,
            zero_struct(SpriteModifiers), tint, shader_handle(TEXTURE_SHADER), 0);
    }
}

void game_ui(Game *game, LinearArena *scratch, const FrameData *frame_data)
{
    Entity *player = world_get_player_entity(&game->world);
    UIState *ui = &game->game_ui.backend_state;
    ASSERT(player);

    if (input_is_key_pressed(&frame_data->input, KEY_I)) {
        game->game_ui.inventory_menu.active = !game->game_ui.inventory_menu.active;
    }

    Entity *hovered_entity =
        es_try_get_entity(&game->world.entity_system, game->game_ui.hovered_entity);

    if (hovered_entity && input_is_key_pressed(&frame_data->input, MOUSE_RIGHT)) {
        if (es_has_component(hovered_entity, InventoryStorable)) {
            // TODO: check that no item is on cursor already
            put_item_on_cursor(&game->game_ui, &game->world, hovered_entity);
        }
    }

    UIStyle style = {0};
    style.font = ui_default_style(ui).font;
    style.text_color = rgba32_mono(0.9f, 1.0f);
    style.background_color = rgba32_mono(0.4f, 0.5f);
    style.background_shadow_color = rgba32_mono(0.7f, 0.5f);
    style.context_menu_color = rgba32_mono(0.8f, 0.5f);
    style.accent_color = rgba32(0.1f, 0.5f, 0.8f, 1.0f);
    style.active_color = rgba32(1.0f, 0.0f, 0.0f, 1.0f);
    style.hot_color = rgba32(0.0f, 1.0f, 0.0f, 1.0f);

    ui_set_next_style(ui, UI_STYLE_TRANSPARENT);
    ui_begin_container(ui, V2_ZERO, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);

    ui_push_style(ui, style);

    inventory_menu(game, frame_data, scratch);

#if 0
    ui_core_same_line(ui);

    equipment_menu(&game->game_ui, game, scratch, frame_data);

    ui_core_same_line(ui);

    spellbook_menu(&game->game_ui, game);
#endif

    ui_pop_container(ui);

    ui_pop_style(ui);
}
