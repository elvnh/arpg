#include "game_ui.h"
#include "base/format.h"
#include "components/component.h"
#include "entity/entity_system.h"
#include "game.h"
#include "item.h"
#include "item_system.h"
#include "magic.h"
#include "modifier.h"
#include "ui/ui_builder.h"
#include "ui/widget.h"
#include "world/world.h"

#define GAME_UI_COLOR (RGBA32) {0, 1, 0, 0.5f}

// TODO: clean up this file

static String item_widget_string(ItemID id, Item *item, LinearArena *scratch)
{
    String id_string = item_id_to_string(id, scratch);
    String result = format(scratch, FMT_STR"##"FMT_STR, FMT_STR_ARG(item->name),
        FMT_STR_ARG(id_string));

    return result;
}

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

		if (interaction.clicked) {
		    ui_state->selected_spellbook_index = i;
		}
	    }

	} ui_end_list(ui);
    } ui_pop_container(ui);

}

static void item_hover_menu(UIState *ui, Item *item, Vector2 mouse_position, LinearArena *arena)
{
    ui_begin_mouse_menu(ui, mouse_position); {
	ui_text(ui, item->name);

	ui_spacing(ui, 12);

	if (item_has_prop(item, ITEM_PROP_HAS_MODIFIERS)) {
	    for (ssize i = 0; i < item->modifiers.modifier_count; ++i) {
		Modifier mod = item->modifiers.modifiers[i];
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

    ui_text(ui, equipment_slot_spelling(slot));
    ui_core_same_line(ui);

    ItemID item_id = get_equipped_item_in_slot(equipment, slot);
    Item *item = item_sys_try_get_item(&game->world.item_system, item_id);

    if (item) {
	String text = item_widget_string(item_id, item, scratch);
	WidgetInteraction interaction = ui_button(ui, text);

	if (interaction.clicked) {
	    bool unequip_success = unequip_item_and_put_in_inventory(equipment, inventory, slot);
	    ASSERT(unequip_success);
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

	EquipmentComponent *eq = es_get_component(player, EquipmentComponent);
	InventoryComponent *inv = es_get_component(player, InventoryComponent);
	ASSERT(eq);
	ASSERT(inv);

	for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
	    equipment_slot_widget(ui_state, game, &eq->equipment, &inv->inventory, slot,
		scratch, &frame_data->input);
	}
    } ui_pop_container(ui);
}

static void inventory_menu(GameUIState *ui_state, Game *game, LinearArena *scratch,
    const FrameData *frame_data)
{
    UIState *ui = &ui_state->backend_state;
    Entity *player = world_get_player_entity(&game->world);

    ui_begin_container(ui, str_lit("inventory"), V2_ZERO, GAME_UI_COLOR, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f); {
	ui_text(ui, str_lit("Inventory"));

	// TODO: allow setting min/max size of list
	ui_begin_list(ui, str_lit("player_inventory")); {
	    InventoryComponent *inv = es_get_component(player, InventoryComponent);
	    EquipmentComponent *eq = es_get_component(player, EquipmentComponent);

	    ASSERT(inv);
	    ASSERT(eq);

	    for (ssize i = 0; i < inv->inventory.item_count; ++i) {
		Item *item = item_sys_get_item(&game->world.item_system, inv->inventory.items[i]);
		ASSERT(item);
		ASSERT(item->name.data);

		ItemID item_id = inv->inventory.items[i];
		String label_string = item_widget_string(item_id, item, scratch);

		WidgetInteraction interaction = ui_selectable(ui, label_string);

		if (interaction.clicked) {
		    equip_item_from_inventory(&game->world.item_system, &eq->equipment,
			&inv->inventory, item_id);
		} else if (interaction.hovered) {
		    item_hover_menu(ui, item, frame_data->input.mouse_position, scratch);
		}
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
