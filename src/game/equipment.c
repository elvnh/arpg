#include "equipment.h"
#include "item.h"

b32 can_equip_item_in_slot(Item *item, EquipmentSlot slot)
{
    b32 item_is_equipment = item_has_prop(item, ITEM_PROP_EQUIPMENT);
    b32 result = item_is_equipment && ((item->equipment.slot & slot) != 0);

    return result;
}

static EquipResult equip_and_replace_item_in_slot(ItemID *slot, ItemID item_to_equip)
{
    EquipResult result = {0};

    result.item_was_replaced = !item_ids_equal(*slot, NULL_ITEM_ID);
    result.replaced_item = *slot;

    *slot = item_to_equip;

    return result;
}

static ItemID *get_pointer_to_item_in_slot(Equipment *eq, EquipmentSlot slot)
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

ItemID get_equipped_item_in_slot(Equipment *eq, EquipmentSlot slot)
{
    ItemID *item = get_pointer_to_item_in_slot(eq, slot);

    return *item;
}

b32 has_item_equipped_in_slot(Equipment *eq, EquipmentSlot slot)
{
    ItemID id = get_equipped_item_in_slot(eq, slot);
    b32 result = !item_ids_equal(id, NULL_ITEM_ID);

    return result;
}

EquipResult equip_item_in_slot(Equipment *eq, ItemID item, EquipmentSlot slot)
{
    // TODO: pass item pointer, get item id from pointer
    //ASSERT(can_equip_item_in_slot(item *item, EquipmentSlot slot))

    ItemID *item_in_slot = get_pointer_to_item_in_slot(eq, slot);
    EquipResult result = equip_and_replace_item_in_slot(item_in_slot, item);

    return result;
}
