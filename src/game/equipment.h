#ifndef EQUIPMENT_H
#define EQUIPMENT_H

typedef struct {
    ItemID head;
} Equipment;

typedef struct {
    b32 item_was_replaced;
    ItemID replaced_item;
} EquipResult;

static inline b32 can_equip_item_in_slot(Item *item, EquipmentSlot slot)
{
    b32 item_is_equipment = item_has_prop(item, ITEM_PROP_EQUIPMENT);
    b32 result = item_is_equipment && ((item->equipment.slot & slot) != 0);

    return result;
}

static inline EquipResult equip_item_in_slot(Equipment *eq, ItemID item, EquipmentSlot slot)
{
    // TODO: pass item pointer, get item id from pointer
    //ASSERT(can_equip_item_in_slot(item *item, EquipmentSlot slot))

    EquipResult result = {0};

    switch (slot) {
        case EQUIP_SLOT_HEAD: {
            result.item_was_replaced = !item_ids_equal(eq->head, NULL_ITEM_ID);
            result.replaced_item = eq->head;
            eq->head = item;
        } break;

        INVALID_DEFAULT_CASE;
    }

    return result;
}

#endif //EQUIPMENT_H
