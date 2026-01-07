#include "game_ui.h"
#include "game.h"
#include "item_system.h"
#include "ui/ui_builder.h"

#define GAME_UI_COLOR (RGBA32) {0, 1, 0, 0.5f}

// TODO: clean up this file

static String item_widget_string(ItemID id, Item *item, GameMemory *memory)
{
    Allocator alloc = la_allocator(&memory->temporary_memory);

    String non_visible_substring = str_concat(str_lit("##"), item_id_to_string(id, alloc), alloc);
    String result = str_concat(item->name, non_visible_substring, alloc);

    return result;
}

static void equipment_slot_widget(GameUIState *ui_state, Game *game, Equipment *equipment,
    Inventory *inventory, EquipmentSlot slot, GameMemory *game_memory, const Input *input)
{
    UIState *ui = &ui_state->backend_state;

    ui_text(ui, equipment_slot_spelling(slot));
    ui_core_same_line(ui);

    ItemID item_id = get_equipped_item_in_slot(equipment, slot);
    Item *item = item_mgr_get_item(&game->world.item_manager, item_id);

    if (item) {
	String text = item_widget_string(item_id, item, game_memory);
	WidgetInteraction interaction = ui_button(ui, text);

	if (interaction.hovered && !interaction.clicked) {
	    ui_begin_mouse_menu(ui, input->mouse_position); {
		ui_text(ui, str_lit("(item stats)"));
	    } ui_end_mouse_menu(ui);
	} else if (interaction.clicked) {
	    bool unequip_success = unequip_item_and_put_in_inventory(equipment, inventory, slot);
	    ASSERT(unequip_success);
	}
    } else {
	ui_non_interactible_button(ui, str_lit("(empty)"));
    }
}

static void equipment_menu(GameUIState *ui_state, Game *game, GameMemory *game_memory,
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
		game_memory, &frame_data->input);
	}
    } ui_pop_container(ui);
}

static void inventory_menu(GameUIState *ui_state, Game *game, GameMemory *game_memory,
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
		Item *item = item_mgr_get_item(&game->world.item_manager, inv->inventory.items[i]);
		ASSERT(item->name.data);

		ItemID item_id = inv->inventory.items[i];
		String label_string = item_widget_string(item_id, item, game_memory);

		ASSERT(item);

		if (ui_selectable(ui, label_string).clicked) {
		    equip_item_from_inventory(&game->world.item_manager, &eq->equipment,
			&inv->inventory, item_id);
		}
	    }
	} ui_end_list(ui);
    } ui_pop_container(ui);
}

void game_ui(Game *game, GameMemory *game_memory, const FrameData *frame_data)
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

    inventory_menu(&game->game_ui, game, game_memory, frame_data);

    ui_core_same_line(ui);

    equipment_menu(&game->game_ui, game, game_memory, frame_data);

    ui_pop_container(ui);
}
