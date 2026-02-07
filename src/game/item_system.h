#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "base/free_list_arena.h"
#include "item.h"

#define MAX_ITEMS 256

/*
  TODO:
  - Item names:
    String interning will probably have to be used since many copies
    of generic items will probably exist.
  - If worlds only represent single levels, how should copying items over
    (eg. in player inventory) to new level work?
  - Dynamic capacity of items
 */

typedef struct {
    Item item;
    u32 generation;
} ItemStorageSlot;

DEFINE_STATIC_RING_BUFFER(ItemID, ItemIDQueue, MAX_ITEMS);

typedef struct ItemSystem {
    ItemStorageSlot item_slots[MAX_ITEMS];
    ItemIDQueue id_queue;
} ItemSystem;

void item_sys_initialize(ItemSystem *item_sys);
ItemWithID item_sys_create_item(ItemSystem *item_sys);
void item_sys_set_item_name(ItemSystem *item_sys, Item *item, String name, LinearArena *world_arena);
void item_sys_destroy_item(ItemSystem *item_sys, ItemID id);
Item *item_sys_get_item(ItemSystem *item_sys, ItemID id);
ItemID item_sys_get_id_of_item(ItemSystem *item_sys, Item *item);

#endif //ITEM_MANAGER_H
