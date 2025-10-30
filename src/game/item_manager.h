#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "item.h"

#define MAX_ITEMS 1024

typedef struct {
    Item item;
    u32 generation;
} ItemStorageSlot;

DEFINE_STATIC_RING_BUFFER(ItemID, ItemIDQueue, MAX_ITEMS);

typedef struct {
    ItemStorageSlot item_slots[MAX_ITEMS];
    ItemIDQueue id_queue;
} ItemManager;

void item_mgr_initialize(ItemManager *item_mgr);
ItemWithID item_mgr_create_item(ItemManager *item_mgr);
void item_mgr_destroy_item(ItemManager *item_mgr, ItemID id);
Item *item_mgr_get_item(ItemManager *item_mgr, ItemID id);
ItemID item_mgr_get_id_of_item(ItemManager *item_mgr, Item *item);

#endif //ITEM_MANAGER_H
