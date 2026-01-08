#ifndef ITEM_H
#define ITEM_H

#include "base/ring_buffer.h"
#include "base/typedefs.h"
#include "base/string8.h"
#include "modifier.h"

#define NULL_ITEM_ID (ItemID){0}

typedef struct {
    u32 id;
    u32 generation;
} ItemID;

typedef enum {
    ITEM_PROP_EQUIPPABLE = (1 << 0),
    ITEM_PROP_HAS_MODIFIERS = (1 << 1),
} ItemProperty;

typedef enum {
    EQUIP_SLOT_HEAD,
    EQUIP_SLOT_LEFT_FINGER,
    EQUIP_SLOT_RIGHT_FINGER,
    EQUIP_SLOT_GLOVES,
    EQUIP_SLOT_BODY,
    EQUIP_SLOT_LEGS,
    EQUIP_SLOT_FEET,
    EQUIP_SLOT_NECK,
    EQUIP_SLOT_WEAPON,

    EQUIP_SLOT_COUNT,

    // NOTE: not an actual slot, only to show that an item is equippable in either left or right finger slot
    EQUIPPABLE_IN_FINGER_SLOT,
} EquipmentSlot;

typedef struct {
    String name;
    ItemProperty properties;

    struct {
        EquipmentSlot slot;
    } equipment;

    struct {
        Modifier modifiers[6];
        s32 modifier_count;
    } modifiers;
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

static inline b32 item_id_is_null(ItemID id)
{
    b32 result = item_ids_equal(id, NULL_ITEM_ID);

    return result;
}

static inline void item_add_modifier(Item *item, Modifier mod)
{
    ASSERT(item_has_prop(item, ITEM_PROP_HAS_MODIFIERS));
    ASSERT(item->modifiers.modifier_count < ARRAY_COUNT(item->modifiers.modifiers));

    s32 index = item->modifiers.modifier_count++;
    item->modifiers.modifiers[index] = mod;
}

static inline String equipment_slot_spelling(EquipmentSlot slot)
{
    switch (slot) {
	case EQUIP_SLOT_HEAD:         return str_lit("Head");
	case EQUIP_SLOT_NECK:         return str_lit("Neck");
	case EQUIP_SLOT_LEFT_FINGER:  return str_lit("Left ring");
	case EQUIP_SLOT_RIGHT_FINGER: return str_lit("Right ring");
	case EQUIP_SLOT_GLOVES:       return str_lit("Hands");
	case EQUIP_SLOT_BODY:         return str_lit("Body");
	case EQUIP_SLOT_LEGS:         return str_lit("Legs");
	case EQUIP_SLOT_FEET:         return str_lit("Feet");
	case EQUIP_SLOT_WEAPON:       return str_lit("Weapon");

	INVALID_DEFAULT_CASE;
    }

    ASSERT(0);
    return (String){0};
}

static inline String item_id_to_string(ItemID id, Allocator alloc)
{
    String a = s64_to_string(id.id, alloc);
    String b = s64_to_string(id.generation, alloc);
    String result = str_concat(
	a,
	str_concat(
	    str_lit(","),
	    b,
	    alloc
	),
	alloc
    );

    return result;
}

#endif //ITEM_H
