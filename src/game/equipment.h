#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "item_manager.h"
#include "inventory.h"

#define NULL_ITEM_ID (ItemID){0}

typedef struct {
    ItemID head;
    ItemID neck;
    ItemID left_finger;
    ItemID right_finger;
    ItemID hands;
    ItemID body;
    ItemID legs;
    ItemID feet;
} Equipment;

ItemID get_equipped_item_in_slot(Equipment *eq, EquipmentSlot slot);
b32    has_item_equipped_in_slot(Equipment *eq, EquipmentSlot slot);
bool   equip_item_from_inventory(ItemManager *item_mgr, Equipment *eq, Inventory *inv, ItemID item);
bool   unequip_item_and_put_in_inventory(Equipment *eq, Inventory *inv, EquipmentSlot slot);

#endif //EQUIPMENT_H
