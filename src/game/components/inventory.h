#ifndef INVENTORY_H
#define INVENTORY_H

#include "entity/entity_id.h"
#include "base/vector.h"

//// TODO: move this to component subdirectory

struct EntitySystem;
struct World;

typedef struct {
    EntityID next_item_in_inventory;
    EntityID prev_item_in_inventory;
} InventoryStorable;

typedef struct {
    EntityID first_item_in_inventory;
    EntityID last_item_in_inventory;
} Inventory;

void append_item_to_inventory(struct EntitySystem *es, Inventory *inv, InventoryStorable *item_to_add);
void insert_item_in_inventory(struct EntitySystem *es, Inventory *inv, InventoryStorable *item_to_add,
			      InventoryStorable *insert_after);
void remove_item_from_inventory(struct EntitySystem *es, Inventory *inv, InventoryStorable *item_to_remove);
void drop_item_on_ground(struct World *world, Inventory *inv, InventoryStorable *item, Vector2 pos);
b32  inventory_contains_item(struct EntitySystem *es, Inventory *inventory, InventoryStorable *item);

static inline b32 inventory_is_empty(Inventory *inventory)
{
    b32 result = entity_id_is_null(inventory->first_item_in_inventory);
    ASSERT((!result || entity_id_is_null(inventory->last_item_in_inventory))
        && "Either both head and tail should be null, or neither should be");

    return result;
}

#endif //INVENTORY_H
