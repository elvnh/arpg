#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "item_manager.h"
#include "inventory.h"

#define NULL_ITEM_ID (ItemID){0}

typedef struct {
    ItemID head;
} Equipment;

typedef struct {
    b32 success;
    ItemID replaced_item;
} EquipResult;

//b32 can_equip_item_in_slot(ItemManager *item_mgr, ItemID item_id, EquipmentSlot slot);
ItemID get_equipped_item_in_slot(Equipment *eq, EquipmentSlot slot);
b32    has_item_equipped_in_slot(Equipment *eq, EquipmentSlot slot);
bool   equip_item_from_inventory_in_slot(ItemManager *item_mgr, Equipment *eq, Inventory *inv,
    ItemID item, EquipmentSlot slot);

#endif //EQUIPMENT_H
