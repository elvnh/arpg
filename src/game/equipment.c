#include "equipment.h"
#include "item.h"
#include "item_manager.h"

static ItemID *get_pointer_to_item_id_in_slot(Equipment *eq, EquipmentSlot slot)
{
    ItemID *result = 0;

    switch (slot) {
        case EQUIP_SLOT_HEAD: {
            result = &eq->head;
        } break;

        INVALID_DEFAULT_CASE;
    }

    return result;
}

b32 can_equip_item_in_slot(ItemManager *item_mgr, ItemID item_id, EquipmentSlot slot)
{
    Item *item = item_mgr_get_item(item_mgr, item_id);

    b32 item_is_equipment = item_has_prop(item, ITEM_PROP_EQUIPMENT);
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

EquipResult try_equip_item_in_slot(ItemManager *item_mgr, Equipment *eq, ItemID item_id, EquipmentSlot slot)
{
    EquipResult result = {0};

    if (can_equip_item_in_slot(item_mgr, item_id, slot)) {
        ItemID *slot_ptr = get_pointer_to_item_id_in_slot(eq, slot);
        result.replaced_item = exchange_item_ids(slot_ptr, item_id);
        result.success = true;
    }

    return result;
}
