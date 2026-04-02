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
    EntityID id = get_equipped_item_id_in_slot(es, eq, slot);
    Entity *result = es_try_get_entity(es, id);

    return result;
}

EntityID get_equipped_item_id_in_slot(struct EntitySystem *es, Equipment *equipment,
    EquipmentSlot slot)
{
    EntityID result = *get_equipped_entity_id_ptr_in_slot(equipment, slot);

    return result;
}

EquipResult can_equip_item_in_any_slot(EntitySystem *es, Equipment *equipment,
    Equippable *equippable)
{
    EquipResult result =
        can_equip_item_in_slot(es, equipment, equippable, equippable->equippable_in_slot);

    return result;
}

EquipResult can_equip_item_in_slot(EntitySystem *es, Equipment *equipment,
    Equippable *equippable, EquipmentSlot slot)
{
    EquipResult result = {0};

    if (equippable->equippable_in_slot == slot) {
        Entity *item_entity = es_get_component_owner(es, equippable, Equippable);

        result.ok = true;
        result.replaced_item = *get_equipped_entity_id_ptr_in_slot(equipment, slot);
    }

    return result;
}

EquipResult try_equip_item_in_any_slot(EntitySystem *es, Equipment *equipment,
    Equippable *equippable)
{
    EquipResult result =
        try_equip_item_in_slot(es, equipment, equippable, equippable->equippable_in_slot);

    return result;
}

EquipResult try_equip_item_in_slot(struct EntitySystem *es, Equipment *equipment,
    Equippable *equippable, EquipmentSlot slot)
{
    EquipResult result = can_equip_item_in_slot(es, equipment, equippable, slot);

    if (result.ok) {
        Entity *item_entity = es_get_component_owner(es, equippable, Equippable);
        *get_equipped_entity_id_ptr_in_slot(equipment, slot) =
            es_get_id_of_entity(es, item_entity);
    }

    return result;
}

b32 try_equip_item_from_inventory(
    EntitySystem *es, Equipment *equipment, Inventory *inventory, InventoryStorable *item)
{
    b32 result = false;

    Entity *item_entity = es_get_component_owner(es, item, InventoryStorable);
    Equippable *equippable = es_try_get_component(item_entity, Equippable);

    if (equippable) {
        EquipResult equip_result = try_equip_item_in_any_slot(es, equipment, equippable);
        result = equip_result.ok;

        if (result) {
            if (!entity_id_is_null(equip_result.replaced_item)) {
                Entity *replaced_item_entity =
                    es_get_entity(es, equip_result.replaced_item);
                InventoryStorable *replaced_item =
                    es_get_component(replaced_item_entity, InventoryStorable);

                try_add_item_to_inventory(es, inventory, replaced_item);
            }

            remove_item_from_inventory(es, inventory, item);
        }
    }

    return result;
}

/* void unequip_item_and_put_in_inventory(EntitySystem *es, Equipment *equipment, Inventory *inventory, EquipmentSlot slot) */
/* { */
/*     EntityID *equipped_id = get_equipped_entity_id_in_slot(equipment, slot); */
/*     Entity *equipped = es_get_entity(es, *equipped_id); */
/*     InventoryStorable *storable = es_get_component(equipped, InventoryStorable); */
/*     ASSERT(storable); */

/*     if (try_add_item_to_inventory(es, inventory, storable)) { */
/*         *equipped_id = NULL_ENTITY_ID; */
/*     } */
/* } */

void unequip_item(EntitySystem *es, Equipment *equipment, EquipmentSlot slot)
{
    EntityID *equipped_id = get_equipped_entity_id_ptr_in_slot(equipment, slot);
    *equipped_id = NULL_ENTITY_ID;
}
