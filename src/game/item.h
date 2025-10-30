#ifndef ITEM_H
#define ITEM_H

#include "base/ring_buffer.h"
#include "base/typedefs.h"

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
    ItemID id;
    Item *item;
} ItemWithID;

static inline b32 item_has_prop(Item *item, ItemProperty property)
{
    b32 result = (item->properties & property) != 0;

    return result;
}

static inline void item_set_prop(Item *item, ItemProperty property)
{
    ASSERT(!item_has_prop(item, property));
    item->properties |= property;
}

static inline b32 item_ids_equal(ItemID lhs, ItemID rhs)
{
    b32 result = (lhs.id == rhs.id) && (lhs.generation == rhs.generation);

    return result;
}

#endif //ITEM_H
