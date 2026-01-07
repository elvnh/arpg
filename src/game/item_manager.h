#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "base/free_list_arena.h"
#include "item.h"

#define MAX_ITEMS 1024

/*
  TODO:
  - Item names:
    String interning will probably have to be used since many copies
    of generic items will probably exist.
  - If worlds only represent single levels, how should copying items over
    (eg. in player inventory) to new level work?
 */

typedef struct {
    Item item;
    u32 generation;
} ItemStorageSlot;

DEFINE_STATIC_RING_BUFFER(ItemID, ItemIDQueue, MAX_ITEMS);

typedef struct ItemManager {
    ItemStorageSlot item_slots[MAX_ITEMS];
    ItemIDQueue id_queue;
    FreeListArena item_system_memory;
} ItemManager;

void item_mgr_initialize(ItemManager *item_mgr, Allocator allocator);
ItemWithID item_mgr_create_item(ItemManager *item_mgr);
void item_mgr_set_item_name(ItemManager *item_mgr, Item *item, String name);
void item_mgr_destroy_item(ItemManager *item_mgr, ItemID id);
Item *item_mgr_get_item(ItemManager *item_mgr, ItemID id);
ItemID item_mgr_get_id_of_item(ItemManager *item_mgr, Item *item);

#endif //ITEM_MANAGER_H
