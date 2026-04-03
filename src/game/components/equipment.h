#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "base/string8.h"
#include "entity/entity_id.h"
#include "inventory.h"

struct Entity;
struct EntitySystem;

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

    // NOTE: not an actual slot, only to show that an item is equippable in
    // either left or right finger slot
    EQUIPPABLE_IN_FINGER_SLOT,
} EquipmentSlot;

typedef struct {
    EntityID items[EQUIP_SLOT_COUNT];
} Equipment;

typedef struct {
    EquipmentSlot equippable_in_slot;
} Equippable;

typedef struct {
    b32 ok;
    EntityID replaced_item;
} EquipResult;

/* Querying operations */
struct Entity *get_equipped_item_in_slot(struct EntitySystem *es, Equipment *equipment,
    EquipmentSlot slot);
EntityID get_equipped_item_id_in_slot(Equipment *equipment, EquipmentSlot slot);
EquipResult can_equip_item_in_any_slot(Equipment *equipment, Equippable *equippable);
EquipResult can_equip_item_in_slot(Equipment *equipment, Equippable *equippable,
    EquipmentSlot slot);

/* Modifying operations */
EquipResult equip_item_in_any_slot(struct EntitySystem *es, Equipment *equipment,
    Equippable *equippable);
EquipResult equip_item_in_slot(struct EntitySystem *es, Equipment *equipment,
    Equippable *equippable, EquipmentSlot slot);
void unequip_item_in_slot(Equipment *equipment, EquipmentSlot slot);

/* Miscellaneous */
static inline String equipment_slot_to_string(EquipmentSlot slot)
{
    // clang-format off
    BEGIN_EXHAUSTIVE_SWITCH;
    switch (slot) {
        case EQUIP_SLOT_HEAD:         return str("Head");
        case EQUIP_SLOT_NECK:         return str("Neck");
        case EQUIP_SLOT_LEFT_FINGER:  return str("Left ring");
        case EQUIP_SLOT_RIGHT_FINGER: return str("Right ring");
        case EQUIP_SLOT_GLOVES:       return str("Hands");
        case EQUIP_SLOT_BODY:         return str("Body");
        case EQUIP_SLOT_LEGS:         return str("Legs");
        case EQUIP_SLOT_FEET:         return str("Feet");
        case EQUIP_SLOT_WEAPON:       return str("Weapon");

        INVALID_CASE(EQUIP_SLOT_COUNT);
        INVALID_CASE(EQUIPPABLE_IN_FINGER_SLOT);
        INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    // clang-format on
    ASSERT(0);
    return (String){0};
}

#endif //EQUIPMENT_H
