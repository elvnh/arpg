#include "equipment.h"

#include "components/component.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "inventory.h"

static EntityID *get_equipped_entity_id_ptr_in_slot(Equipment *eq, EquipmentSlot slot)
{
    ASSERT(slot >= 0);
    ASSERT(slot < EQUIP_SLOT_COUNT);

    EntityID *result = &eq->items[slot];

    return result;
}

Entity *get_equipped_item_in_slot(EntitySystem *es, Equipment *eq, EquipmentSlot slot)
{
    EntityID id = get_equipped_item_id_in_slot(eq, slot);
    Entity *result = es_try_get_entity(es, id);

    return result;
}

EntityID get_equipped_item_id_in_slot(Equipment *equipment, EquipmentSlot slot)
{
    EntityID result = *get_equipped_entity_id_ptr_in_slot(equipment, slot);

    return result;
}

EquipResult can_equip_item_in_any_slot(Equipment *equipment, Equippable *equippable)
{
    EquipResult result =
        can_equip_item_in_slot(equipment, equippable, equippable->equippable_in_slot);

    return result;
}

EquipResult can_equip_item_in_slot(Equipment *equipment, Equippable *equippable,
    EquipmentSlot slot)
{
    EquipResult result = {0};

    if (equippable->equippable_in_slot == slot) {
        result.ok = true;
        result.replaced_item = *get_equipped_entity_id_ptr_in_slot(equipment, slot);
    }

    return result;
}

EquipResult equip_item_in_any_slot(EntitySystem *es, Equipment *equipment,
    Equippable *equippable)
{
    EquipResult result =
        equip_item_in_slot(es, equipment, equippable, equippable->equippable_in_slot);

    return result;
}

EquipResult equip_item_in_slot(struct EntitySystem *es, Equipment *equipment,
    Equippable *equippable, EquipmentSlot slot)
{
    EquipResult result = can_equip_item_in_slot(equipment, equippable, slot);
    ASSERT(result.ok);

    Entity *item_entity = es_get_component_owner(es, equippable, Equippable);
    *get_equipped_entity_id_ptr_in_slot(equipment, slot) =
        es_get_id_of_entity(es, item_entity);

    return result;
}

void unequip_item_in_slot(Equipment *equipment, EquipmentSlot slot)
{
    EntityID *equipped_id = get_equipped_entity_id_ptr_in_slot(equipment, slot);
    *equipped_id = NULL_ENTITY_ID;
}
