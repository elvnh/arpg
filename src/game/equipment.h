#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "item.h"

#define NULL_ITEM_ID (ItemID){0}

typedef struct {
    ItemID head;
} Equipment;

typedef struct {
    b32 item_was_replaced;
    ItemID replaced_item;
} EquipResult;

b32 can_equip_item_in_slot(Item *item, EquipmentSlot slot);
EquipResult equip_item_in_slot(Equipment *eq, ItemID item, EquipmentSlot slot);

#endif //EQUIPMENT_H
