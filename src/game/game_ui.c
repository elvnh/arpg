#include "game_ui.h"
#include "game.h"

#define GAME_UI_COLOR (RGBA32) {0, 1, 0, 0.5f}

// TODO: clean up this file

static void equipment_slot_widget(GameUIState *ui_state, GameState *game_state, Equipment *equipment,
    Inventory *inventory, EquipmentSlot slot)
{
    UIState *ui = &ui_state->backend_state;

    ui_text(ui, equipment_slot_spelling(slot));
    ui_core_same_line(ui);

    ItemID item_id = get_equipped_item_in_slot(equipment, slot);
    Item *item = item_mgr_get_item(&game_state->world.item_manager, item_id);

    if (item) {
	if (ui_button(ui, item->name).clicked) {
	    bool unequip_success = unequip_item_and_put_in_inventory(equipment, inventory, slot);
	    ASSERT(unequip_success);
	}
    } else {
	ui_non_interactible_button(ui, str_lit("(empty)"));
    }
}

static void equipment_menu(GameUIState *ui_state, GameState *game_state, GameMemory *game_memory,
    const FrameData *frame_data)
{
    UIState *ui = &ui_state->backend_state;
    Entity *player = world_get_player_entity(&game_state->world);

    ui_begin_container(ui, str_lit("equipment"), V2_ZERO, GAME_UI_COLOR, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f); {
	ui_text(ui, str_lit("Equipment"));

	EquipmentComponent *eq = es_get_component(player, EquipmentComponent);
	InventoryComponent *inv = es_get_component(player, InventoryComponent);
	ASSERT(eq);
	ASSERT(inv);

	for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
	    equipment_slot_widget(ui_state, game_state, &eq->equipment, &inv->inventory, slot);
	}
    } ui_pop_container(ui);
}

static void inventory_menu(GameUIState *ui_state, GameState *game_state, GameMemory *game_memory,
    const FrameData *frame_data)
{
    UIState *ui = &ui_state->backend_state;
    Entity *player = world_get_player_entity(&game_state->world);

    ui_begin_container(ui, str_lit("inventory"), V2_ZERO, GAME_UI_COLOR, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f); {

	ui_text(ui, str_lit("Inventory"));

	// TODO: allow setting min/max size of list
	ui_begin_list(ui, str_lit("player_inventory")); {
	    InventoryComponent *inv = es_get_component(player, InventoryComponent);
	    EquipmentComponent *eq = es_get_component(player, EquipmentComponent);

	    ASSERT(inv);
	    ASSERT(eq);

	    for (ssize i = 0; i < inv->inventory.item_count; ++i) {
		Item *item = item_mgr_get_item(&game_state->world.item_manager, inv->inventory.items[i]);
		ItemID item_id = inv->inventory.items[i];
		ASSERT(item);

		if (ui_selectable(ui, item->name).clicked) {
		    equip_item_from_inventory(&game_state->world.item_manager, &eq->equipment,
			&inv->inventory, item_id);
		}
	    }
	} ui_end_list(ui);
    } ui_pop_container(ui);
}


void game_ui(GameState *game_state, GameMemory *game_memory, const FrameData *frame_data)
{
    Entity *player = world_get_player_entity(&game_state->world);
    UIState *ui = &game_state->game_ui.backend_state;
    ASSERT(player);

    if (input_is_key_pressed(&frame_data->input, KEY_I)) {
	game_state->game_ui.inventory_menu_open = !game_state->game_ui.inventory_menu_open;
    }

    if (!game_state->game_ui.inventory_menu_open) {
	return;
    }

    ui_begin_container(ui, str_lit("root"), V2_ZERO, RGBA32_TRANSPARENT, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);

    inventory_menu(&game_state->game_ui, game_state, game_memory, frame_data);

    ui_core_same_line(ui);

    equipment_menu(&game_state->game_ui, game_state, game_memory, frame_data);

    ui_pop_container(ui);
}
