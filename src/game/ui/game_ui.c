#include "game_ui.h"

#include "base/format.h"
#include "base/rectangle.h"
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
            ASSERT(entity_id_is_null(game->game_ui.inventory_menu.item_on_cursor)
                   && "TODO: allow exchanging item on cursor with one on ground");
            pick_up_item_from_world_and_put_on_cursor(
                &game->game_ui.inventory_menu, &game->world, hovered_entity);
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

    inventory_menu(ui, &game->game_ui.inventory_menu, &game->world, frame_data, scratch);

#if 0
    ui_core_same_line(ui);

    equipment_menu(&game->game_ui, game, scratch, frame_data);

    ui_core_same_line(ui);

    spellbook_menu(&game->game_ui, game);
#endif

    ui_pop_container(ui);

    ui_pop_style(ui);
}
