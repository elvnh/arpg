#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "base/free_list_arena.h"
#include "base/typedefs.h"
#include "generational_id.h"
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
  - Component system for items?
 */

typedef struct {
    DEFINE_GENERATIONAL_ID_NODE_FIELDS(ItemIndex, ItemGeneration);
} ItemIDSlot;

typedef struct ItemSystem {
    Item        items[MAX_ITEMS];
    ItemIDSlot  item_ids[MAX_ITEMS];
    DEFINE_GENERATIONAL_ID_LIST_HEAD(ItemIndex);
} ItemSystem;

void item_sys_initialize(ItemSystem *item_sys);
ItemWithID item_sys_create_item(ItemSystem *item_sys);
void item_sys_set_item_name(Item *item, String name, LinearArena *world_arena);
void item_sys_destroy_item(ItemSystem *item_sys, ItemID id);
Item *item_sys_get_item(ItemSystem *item_sys, ItemID id);
Item *item_sys_try_get_item(ItemSystem *item_sys, ItemID id);
b32  item_sys_item_exists(ItemSystem *item_sys, ItemID item_id);

#endif //ITEM_MANAGER_H
