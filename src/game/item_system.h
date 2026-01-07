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

typedef struct ItemSystem {
    ItemStorageSlot item_slots[MAX_ITEMS];
    ItemIDQueue id_queue;
    FreeListArena item_system_memory;
} ItemSystem;

void item_mgr_initialize(ItemSystem *item_mgr, Allocator allocator);
ItemWithID item_mgr_create_item(ItemSystem *item_mgr);
void item_mgr_set_item_name(ItemSystem *item_mgr, Item *item, String name);
void item_mgr_destroy_item(ItemSystem *item_mgr, ItemID id);
Item *item_mgr_get_item(ItemSystem *item_mgr, ItemID id);
ItemID item_mgr_get_id_of_item(ItemSystem *item_mgr, Item *item);

#endif //ITEM_MANAGER_H
