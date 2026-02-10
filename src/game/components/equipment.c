#include "equipment.h"
#include "components/component.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "inventory.h"

typedef struct {
    EntityID replaced_item;
    b32 success;
} EquipResult;

static EntityID *get_equipped_entity_id_in_slot(Equipment *eq, EquipmentSlot slot)
{
    ASSERT(slot >= 0);
    ASSERT(slot < EQUIP_SLOT_COUNT);

    EntityID *result = &eq->items[slot];

    return result;
}

Entity *get_equipped_item_in_slot(EntitySystem *es, Equipment *eq, EquipmentSlot slot)
{
    EntityID id = *get_equipped_entity_id_in_slot(eq, slot);
    Entity *result = es_try_get_entity(es, id);

    return result;
}

static EquipResult try_equip_item(EntitySystem *es, Equipment *equipment, Equippable *equippable)
{
    EquipResult result = {0};
    EquipmentSlot slot = equippable->equippable_in_slot;

    Entity *item_entity = es_get_component_owner(es, equippable, Equippable);

    result.replaced_item = *get_equipped_entity_id_in_slot(equipment, slot);
    *get_equipped_entity_id_in_slot(equipment, slot) = item_entity->id;

    result.success = true;

    return result;
}

b32 try_equip_item_from_inventory(EntitySystem *es, Equipment *equipment, Inventory *inventory,
    InventoryStorable *item)
{
    b32 result = false;

    Entity *item_entity = es_get_component_owner(es, item, InventoryStorable);
    Equippable *equippable = es_get_component(item_entity, Equippable);

    if (equippable) {
        EquipResult equip_result = try_equip_item(es, equipment, equippable);

        if (equip_result.success) {
            if (!entity_id_is_null(equip_result.replaced_item)) {
                Entity *replaced_item_entity = es_get_entity(es, equip_result.replaced_item);
                InventoryStorable *replaced_item = es_get_component(replaced_item_entity, InventoryStorable);

                insert_item_in_inventory(es, inventory, replaced_item, item);
            }

            remove_item_from_inventory(es, inventory, item);
        }
    }

    return result;
}

void unequip_item_and_put_in_inventory(EntitySystem *es, Equipment *equipment,
    Inventory *inventory, EquipmentSlot slot)
{
    EntityID *equipped_id = get_equipped_entity_id_in_slot(equipment, slot);
    Entity *equipped = es_get_entity(es, *equipped_id);
    InventoryStorable *storable = es_get_component(equipped, InventoryStorable);
    ASSERT(storable);

    *equipped_id = NULL_ENTITY_ID;

    append_item_to_inventory(es, inventory, storable);
}
