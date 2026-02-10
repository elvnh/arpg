#include "inventory.h"
#include "components/component.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "world/world.h"

// TODO: InventoryStorable -> InventoryItem
// TODO: reduce code duplication between append and insert

static InventoryStorable *get_inventory_item(EntitySystem *es, EntityID id)
{
    Entity *entity = es_try_get_entity(es, id);
    InventoryStorable *result = 0;

    if (entity) {
        result = es_get_component(entity, InventoryStorable);
        ASSERT(result);
    }

    return result;
}

void append_item_to_inventory(EntitySystem *es, Inventory *inv, InventoryStorable *item_to_add)
{
    ASSERT(!inventory_contains_item(es, inv, item_to_add));
    ASSERT(entity_id_is_null(item_to_add->next_item_in_inventory));
    ASSERT(entity_id_is_null(item_to_add->prev_item_in_inventory));

    Entity *inventory_owner = es_get_component_owner(es, inv, Inventory);
    Entity *item_entity = es_get_component_owner(es, item_to_add, InventoryStorable);

    ASSERT(inventory_owner != item_entity);
    ASSERT(es_has_component(inventory_owner, Inventory));
    ASSERT(es_has_component(item_entity, InventoryStorable));

    if (entity_id_is_null(inv->first_item_in_inventory)) {
        ASSERT(entity_id_is_null(inv->last_item_in_inventory));

        inv->first_item_in_inventory = item_entity->id;
        inv->last_item_in_inventory = item_entity->id;
    } else {
        InventoryStorable *last_item = get_inventory_item(es, inv->last_item_in_inventory);

        insert_item_in_inventory(es, inv, item_to_add, last_item);
    }

    // Items in inventory should no longer be visible in world
    es_remove_component(item_entity, PhysicsComponent);
}

void insert_item_in_inventory(EntitySystem *es, Inventory *inv, InventoryStorable *item_to_add,
    InventoryStorable *insert_after)
{
    ASSERT(insert_after && "Should never be called on empty inventory");
    ASSERT(!inventory_contains_item(es, inv, item_to_add));
    ASSERT(!entity_id_is_null(inv->first_item_in_inventory));
    ASSERT(!entity_id_is_null(inv->last_item_in_inventory));
    ASSERT(entity_id_is_null(item_to_add->next_item_in_inventory));
    ASSERT(entity_id_is_null(item_to_add->prev_item_in_inventory));
    ASSERT(item_to_add != insert_after);

    Entity *item_to_add_entity = es_get_component_owner(es, item_to_add, InventoryStorable);
    Entity *insert_after_entity = es_get_component_owner(es, insert_after, InventoryStorable);

    InventoryStorable *next_item = get_inventory_item(es, insert_after->next_item_in_inventory);

    if (next_item) {
        next_item->prev_item_in_inventory = item_to_add_entity->id;
    }

    item_to_add->prev_item_in_inventory = insert_after_entity->id;
    item_to_add->next_item_in_inventory = insert_after->next_item_in_inventory;

    insert_after->next_item_in_inventory = item_to_add_entity->id;

    b32 inserted_last = entity_id_equal(inv->last_item_in_inventory, insert_after_entity->id);

    if (inserted_last) {
        inv->last_item_in_inventory = item_to_add_entity->id;
    }

    // Items in inventory should no longer be visible in world
    es_remove_component(item_to_add_entity, PhysicsComponent);
}

void remove_item_from_inventory(EntitySystem *es, Inventory *inv, InventoryStorable *item_to_remove)
{
    ASSERT(inventory_contains_item(es, inv, item_to_remove));

    Entity *prev = es_try_get_entity(es, item_to_remove->prev_item_in_inventory);
    Entity *next = es_try_get_entity(es, item_to_remove->next_item_in_inventory);

    if (prev) {
        InventoryStorable *prev_item = es_get_component(prev, InventoryStorable);
        prev_item->next_item_in_inventory = item_to_remove->next_item_in_inventory;
    }

    if (next) {
        InventoryStorable *next_item = es_get_component(next, InventoryStorable);
        next_item->prev_item_in_inventory = item_to_remove->prev_item_in_inventory;
    }

    Entity *item_entity = es_get_component_owner(es, item_to_remove, InventoryStorable);

    if (entity_id_equal(inv->first_item_in_inventory, item_entity->id)) {
        inv->first_item_in_inventory = item_to_remove->next_item_in_inventory;
    }

    if (entity_id_equal(inv->last_item_in_inventory, item_entity->id)) {
        inv->last_item_in_inventory = item_to_remove->prev_item_in_inventory;
    }

    *item_to_remove = zero_struct(InventoryStorable);
}

void drop_item_on_ground(EntitySystem *es, Inventory *inv, InventoryStorable *item, Vector2 pos)
{
    ASSERT(inventory_contains_item(es, inv, item));

    Entity *item_entity = es_get_component_owner(es, item, InventoryStorable);
    ASSERT(!es_has_component(item_entity, PhysicsComponent));

    PhysicsComponent *physics = es_add_component(item_entity, PhysicsComponent);
    physics->position = pos;

    remove_item_from_inventory(es, inv, item);
}

b32 inventory_contains_item(EntitySystem *es, Inventory *inventory, InventoryStorable *item)
{
    // TODO: keep a ID from item to inventory, or in general from entity to it's parent
    b32 result = false;

    Entity *item_entity = es_get_component_owner(es, item, InventoryStorable);
    EntityID searched_id = item_entity->id;

    EntityID curr_id = inventory->first_item_in_inventory;

    while (!entity_id_is_null(curr_id)) {
        Entity *curr_entity = es_get_entity(es, curr_id);
        InventoryStorable *curr_item = es_get_component(curr_entity, InventoryStorable);

        if (entity_id_equal(curr_entity->id, searched_id)) {
            result = true;
            break;
        }

        curr_id = curr_item->next_item_in_inventory;
    }

    return result;
}
