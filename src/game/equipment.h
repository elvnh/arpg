#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "item_manager.h"

#define NULL_ITEM_ID (ItemID){0}

typedef struct {
    ItemID head;
} Equipment;

typedef struct {
    b32 item_was_replaced;
    ItemID replaced_item; // TODO: use null item id to signify that item wasn't replaced
} EquipResult;

b32 can_equip_item_in_slot(ItemManager *item_mgr, ItemID item_id, EquipmentSlot slot);
EquipResult equip_item_in_slot(ItemManager *item_mgr, Equipment *eq, ItemID item, EquipmentSlot slot);
ItemID get_equipped_item_in_slot(Equipment *eq, EquipmentSlot slot);
b32    has_item_equipped_in_slot(Equipment *eq, EquipmentSlot slot);

#endif //EQUIPMENT_H
