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
#include "platform/input_event.h"
#include "renderer/frontend/render_batch.h"
#include "ui/ui_builder.h"
#include "ui/widget.h"
#include "world/world.h"

// TODO: clean up this file
static void spellbook_menu(GameUI *ui_state, Game *game)
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

static void equipment_slot_widget(GameUI *ui_state, Game *game, Equipment *equipment,
    Inventory *inventory, EquipmentSlot slot, InputEvents *input, LinearArena *scratch)
{
    UIState *ui = &ui_state->backend_state;

    ui_text(ui, equipment_slot_to_string(slot));
    ui_core_same_line(ui);

    Entity *item = get_equipped_item_in_slot(&game->world.entity_system, equipment, slot);

    if (item) {
        String text = get_item_name_widget_text(item, scratch);
        WidgetInteraction interaction = ui_button(ui, text);

        if (interaction.clicked) {
            unequip_item_and_put_in_inventory(
                &game->world.entity_system, equipment, inventory, slot);
        } else if (interaction.hovered) {
            Vector2 mouse_pos = get_mouse_pos(input);
            item_hover_menu(ui, item, mouse_pos, scratch);
        }
    } else {
        ui_non_interactible_button(ui, str("(empty)"));
    }
}

static void equipment_menu(GameUI *ui_state, Game *game, LinearArena *scratch,
    InputEvents *input)
{
    UIState *ui = &ui_state->backend_state;
    Entity *player = world_get_player_entity(&game->world);

    ui_begin_menu(ui, V2_ZERO, str("equipment_container"), UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);
    {
        ui_text(ui, str("Equipment"));

        Equipment *eq = es_get_component(player, Equipment);
        Inventory *inv = es_get_component(player, Inventory);

        for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
            equipment_slot_widget(ui_state, game, eq, inv, slot, input, scratch);
        }
    }
    ui_pop_container(ui);
}

void game_ui(Game *game, LinearArena *scratch, InputEvents *input, CommandQueue *commands)
{
    Entity *player = world_get_player_entity(&game->world);
    UIState *ui = &game->game_ui.backend_state;
    ASSERT(player);

    if (consume_key_pressed(input, KEY_I)) {
        game->game_ui.inventory_menu.active = !game->game_ui.inventory_menu.active;
    }

    inventory_menu_update(&game->game_ui.inventory_menu);

    if (game->game_ui.inventory_menu.active) {
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

        inventory_menu(ui, &game->game_ui.inventory_menu, &game->world, input, commands,
            scratch);

        ui_core_same_line(ui);

        equipment_menu(&game->game_ui, game, scratch, input);

        ui_core_same_line(ui);

        spellbook_menu(&game->game_ui, game);

        ui_pop_container(ui);

        ui_pop_style(ui);
    }
}
