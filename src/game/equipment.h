#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "item_system.h"
#include "inventory.h"

#define NULL_ITEM_ID (ItemID){0}

typedef struct {
    ItemID items[EQUIP_SLOT_COUNT];
} Equipment;

ItemID get_equipped_item_in_slot(Equipment *eq, EquipmentSlot slot);
b32    has_item_equipped_in_slot(Equipment *eq, EquipmentSlot slot);
bool   equip_item_from_inventory(ItemSystem *item_sys, Equipment *eq, Inventory *inv, ItemID item);
bool   unequip_item_and_put_in_inventory(Equipment *eq, Inventory *inv, EquipmentSlot slot);

#endif //EQUIPMENT_H
