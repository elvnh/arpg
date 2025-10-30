#ifndef ITEM_H
#define ITEM_H

#include "base/ring_buffer.h"
#include "base/typedefs.h"

#define MAX_ITEMS 1024
#define NULL_ITEM_ID (ItemID){0}

typedef struct {
    u32 id;
    u32 generation;
} ItemID;

typedef enum {
    ITEM_PROP_EQUIPMENT = (1 << 0),
} ItemProperty;

typedef enum {
    EQUIP_SLOT_HEAD = (1 << 0),
    EQUIP_SLOT_LEFT_HAND = (1 << 1),
    EQUIP_SLOT_RIGHT_HAND = (1 << 2),
    EQUIP_SLOT_HANDS = (EQUIP_SLOT_LEFT_HAND | EQUIP_SLOT_RIGHT_HAND),
} EquipmentSlot;

typedef struct {
    ItemProperty properties;

    struct {
        EquipmentSlot slot;
    } equipment;
} Item;

typedef struct {
    Item item;
    u32 generation;
} ItemStorageSlot;

typedef struct {
    ItemID id;
    Item *item;
} ItemWithID;

DEFINE_STATIC_RING_BUFFER(ItemID, ItemIDQueue, MAX_ITEMS);

typedef struct {
    ItemStorageSlot item_slots[MAX_ITEMS];
    ItemIDQueue id_queue;
} ItemManager;

void item_mgr_initialize(ItemManager *item_mgr);
ItemWithID item_mgr_create_item(ItemManager *item_mgr);
void item_mgr_destroy_item(ItemManager *item_mgr, ItemID id);
Item *item_mgr_get_item(ItemManager *item_mgr, ItemID id);
ItemID item_mgr_get_id_of_item(Item *item);

void item_set_prop(Item *item, ItemProperty property);
b32 item_has_prop(Item *item, ItemProperty property);
b32 item_ids_equal(ItemID lhs, ItemID rhs);

#endif //ITEM_H
