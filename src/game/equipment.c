#include "equipment.h"
#include "item.h"
#include "item_manager.h"

typedef struct {
    b32 success;
    ItemID replaced_item;
} EquipResult;

static ItemID *get_pointer_to_item_id_in_slot(Equipment *eq, EquipmentSlot slot)
{
    ASSERT(slot < EQUIP_SLOT_COUNT);

    ItemID *result = &eq->items[slot];

    return result;
}

b32 can_equip_item_in_slot(ItemManager *item_mgr, ItemID item_id, EquipmentSlot slot)
{
    Item *item = item_mgr_get_item(item_mgr, item_id);

    b32 item_is_equipment = item_has_prop(item, ITEM_PROP_EQUIPPABLE);
    b32 result = item_is_equipment && ((item->equipment.slot & slot) != 0);

    return result;
}

ItemID get_equipped_item_in_slot(Equipment *eq, EquipmentSlot slot)
{
    ItemID *item = get_pointer_to_item_id_in_slot(eq, slot);

    return *item;
}

b32 has_item_equipped_in_slot(Equipment *eq, EquipmentSlot slot)
{
    ItemID id = get_equipped_item_in_slot(eq, slot);
    b32 result = !item_ids_equal(id, NULL_ITEM_ID);

    return result;
}

static ItemID exchange_item_ids(ItemID *old, ItemID new_value)
{
    ItemID old_value = *old;
    *old = new_value;

    return old_value;
}

static EquipResult try_equip_item_in_slot(ItemManager *item_mgr, Equipment *eq, ItemID item_id, EquipmentSlot slot)
{
    EquipResult result = {0};

    if (can_equip_item_in_slot(item_mgr, item_id, slot)) {
        ItemID *slot_ptr = get_pointer_to_item_id_in_slot(eq, slot);
        result.replaced_item = exchange_item_ids(slot_ptr, item_id);
        result.success = true;
    }

    return result;
}

bool equip_item_from_inventory(ItemManager *item_mgr, Equipment *eq, Inventory *inv, ItemID item_id)
{
    ASSERT(inventory_contains_item(inv, item_id));

    Item *item = item_mgr_get_item(item_mgr, item_id);
    ASSERT(item_has_prop(item, ITEM_PROP_EQUIPPABLE));

    EquipResult equip_result = try_equip_item_in_slot(item_mgr, eq, item_id, item->equipment.slot);

    if (equip_result.success) {
	if (!item_id_is_null(equip_result.replaced_item)) {
	    // TODO: how to handle if we can't add the item?
	    inventory_add_item(inv, equip_result.replaced_item);
	}

	inventory_remove_item(inv, item_id);
    }

    return equip_result.success;
}

static ItemID unequip_item_in_slot(Equipment *eq, EquipmentSlot slot)
{
    ItemID *equipped_item_ptr = get_pointer_to_item_id_in_slot(eq, slot);
    ASSERT(equipped_item_ptr);

    ItemID unequipped_item = *equipped_item_ptr;

    exchange_item_ids(equipped_item_ptr, NULL_ITEM_ID);

    return unequipped_item;
}

bool unequip_item_and_put_in_inventory(Equipment *eq, Inventory *inv, EquipmentSlot slot)
{
    ItemID unequipped_item = unequip_item_in_slot(eq, slot);
    bool success = !item_id_is_null(unequipped_item);

    if (success) {
	inventory_add_item(inv, unequipped_item);
    }

    return success;
}
