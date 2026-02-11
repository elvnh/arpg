#include "game_ui.h"
#include "base/format.h"
#include "base/string8.h"
#include "components/component.h"
#include "components/name.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "game.h"
#include "components/equipment.h"
#include "magic.h"
#include "components/modifier.h"
#include "platform/input.h"
#include "ui/ui_builder.h"
#include "ui/widget.h"
#include "world/world.h"

#define GAME_UI_COLOR (RGBA32) {0, 1, 0, 0.5f}

// TODO: clean up this file

static void spellbook_menu(GameUIState *ui_state, Game *game)
{
    UIState *ui = &ui_state->backend_state;

    ui_begin_container(ui, str_lit("spellbook"), V2_ZERO, GAME_UI_COLOR, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f); {
	ui_text(ui, str_lit("Spellbook"));

	ui_begin_list(ui, str_lit("spells")); {
	    Entity *player = world_get_player_entity(&game->world);
	    SpellCasterComponent *spellcaster = es_get_component(player, SpellCasterComponent);
	    ASSERT(spellcaster);

	    for (ssize i = 0; i < spellcaster->spell_count; ++i) {
		String spell_name = spell_type_to_string(spellcaster->spellbook[i]);
		WidgetInteraction interaction = ui_selectable(ui, spell_name);

		// TODO: print info about spell
		if (interaction.clicked) {
		    ui_state->selected_spellbook_index = i;
		}
	    }

	} ui_end_list(ui);
    } ui_pop_container(ui);

}

static String get_item_widget_text(Entity *item_entity, LinearArena *arena)
{
    String result = {0};

    NameComponent *name = es_get_component(item_entity, NameComponent);

    if (name) {
        // Append the item ID to ensure that there are no widget ID collisions
        result = format(arena, "%.*s##%d,%d", name->length, name->data,
            item_entity->id.index, item_entity->id.generation);
    } else {
        result = str_lit("(unnamed item)");
    }

    return result;
}

static void item_hover_menu(UIState *ui, Entity *item, Vector2 mouse_position, LinearArena *arena)
{
    ASSERT(es_has_component(item, InventoryStorable));

    ui_begin_mouse_menu(ui, mouse_position); {
	ui_text(ui, get_item_widget_text(item, arena));

	ui_spacing(ui, 12);

	ItemModifiers *mods = es_get_component(item, ItemModifiers);

	if (mods) {
	    for (ssize i = 0; i < mods->modifier_count; ++i) {
		Modifier mod = mods->modifiers[i];
		String mod_string = modifier_to_string(mod, arena);

		ui_text(ui, mod_string);
		ui_spacing(ui, 12);
	    }
	}

    } ui_end_mouse_menu(ui);
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
            unequip_item_and_put_in_inventory(&game->world.entity_system, equipment, inventory, slot);
	} else if (interaction.hovered) {
	    item_hover_menu(ui, item, input->mouse_position, scratch);
	}
    } else {
	ui_non_interactible_button(ui, str_lit("(empty)"));
    }
}

static void equipment_menu(GameUIState *ui_state, Game *game, LinearArena *scratch,
    const FrameData *frame_data)
{
    UIState *ui = &ui_state->backend_state;
    Entity *player = world_get_player_entity(&game->world);

    ui_begin_container(ui, str_lit("equipment"), V2_ZERO, GAME_UI_COLOR, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f); {
	ui_text(ui, str_lit("Equipment"));

	Equipment *eq = es_get_component(player, Equipment);
	Inventory *inv = es_get_component(player, Inventory);
	ASSERT(eq);
	ASSERT(inv);

        for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
	    equipment_slot_widget(ui_state, game, eq, inv, slot,
		scratch, &frame_data->input);

        }
    } ui_pop_container(ui);
}

static void inventory_menu(GameUIState *ui_state, Game *game, LinearArena *scratch,
    const FrameData *frame_data)
{
    UIState *ui = &ui_state->backend_state;
    Entity *player = world_get_player_entity(&game->world);
    PhysicsComponent *player_physics = es_get_component(player, PhysicsComponent);
    ASSERT(player_physics);

    Vector2 mouse_pos = input_get_mouse_pos(&frame_data->input, Y_IS_DOWN, frame_data->window_size);

    ui_begin_container(ui, str_lit("inventory"), V2_ZERO, GAME_UI_COLOR, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f); {
	ui_text(ui, str_lit("Inventory"));

	// TODO: allow setting min/max size of list
	ui_begin_list(ui, str_lit("player_inventory")); {
            Inventory *inv = es_get_component(player, Inventory);
	    Equipment *eq = es_get_component(player, Equipment);
            ASSERT(inv);
            ASSERT(eq);

            EntityID curr_id = inv->first_item_in_inventory;

            while (!entity_id_is_null(curr_id)) {
                Entity *curr_entity = es_get_entity(&game->world.entity_system, curr_id);
                ASSERT(curr_entity);
                InventoryStorable *inv_storable = es_get_component(curr_entity, InventoryStorable);
                ASSERT(inv_storable);

                Equippable *equippable = es_get_component(curr_entity, Equippable);
                ASSERT(equippable);

                String label_string = get_item_widget_text(curr_entity, scratch);

                WidgetInteraction interaction = ui_selectable(ui, label_string);

		if (interaction.clicked) {
                    try_equip_item_from_inventory(&game->world.entity_system, eq, inv, inv_storable);
		} else if (interaction.hovered) {
		    item_hover_menu(ui, curr_entity, mouse_pos, scratch);
		}

                curr_id = inv_storable->next_item_in_inventory;
            }
	} ui_end_list(ui);
    } ui_pop_container(ui);
}

void game_ui(Game *game, LinearArena *scratch, const FrameData *frame_data)
{
    Entity *player = world_get_player_entity(&game->world);
    UIState *ui = &game->game_ui.backend_state;
    ASSERT(player);

    if (input_is_key_pressed(&frame_data->input, KEY_I)) {
	game->game_ui.inventory_menu_open = !game->game_ui.inventory_menu_open;
    }

    if (!game->game_ui.inventory_menu_open) {
	return;
    }

    ui_begin_container(ui, str_lit("root"), V2_ZERO, RGBA32_TRANSPARENT, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);

    inventory_menu(&game->game_ui, game, scratch, frame_data);

    ui_core_same_line(ui);

    equipment_menu(&game->game_ui, game, scratch, frame_data);

    ui_core_same_line(ui);

    spellbook_menu(&game->game_ui, game);

    ui_pop_container(ui);
}
