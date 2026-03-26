#ifndef INVENTORY_H
#define INVENTORY_H

#include "base/vector.h"
#include "entity/entity_id.h"

#define INVENTORY_GRID_CELL_COUNTS (Vector2i){16, 8}

struct EntitySystem;
struct World;

typedef struct {
    EntityID next_item_in_inventory;
    EntityID prev_item_in_inventory;
    Vector2i inventory_grid_position;
    Vector2i inventory_grid_size;
} InventoryStorable;

typedef struct {
    EntityID first_item_in_inventory;
    EntityID last_item_in_inventory;
} Inventory;

typedef struct {
    b32 ok;
    EntityID exchanged_item;
} InventoryInsertion;

void append_item_to_inventory_list(
    struct EntitySystem *es, Inventory *inv, InventoryStorable *item_to_add);
b32 try_add_item_to_inventory_at(
    struct EntitySystem *es, Inventory *inv, InventoryStorable *item, Vector2i grid_position);
b32 try_add_item_to_inventory(
    struct EntitySystem *es, Inventory *inv, InventoryStorable *item);
void remove_item_from_inventory(
    struct EntitySystem *es, Inventory *inv, InventoryStorable *item_to_remove);
void drop_item_from_inventory_on_ground(
    struct EntitySystem *es, Inventory *inv, InventoryStorable *item);
b32 inventory_contains_item(
    struct EntitySystem *es, Inventory *inventory, InventoryStorable *item);
InventoryInsertion try_place_or_exchange_inventory_item(
    struct EntitySystem *es, Inventory *inventory, InventoryStorable *item, Vector2i grid_pos);

b32 item_is_in_bounds_of_inventory_grid(Vector2i item_grid_coords, Vector2i item_grid_size);
b32 cell_is_in_bounds_of_inventory_grid(Vector2i cell_coords);

b32 can_place_item_in_inventory_at(
    struct EntitySystem *es, Inventory *inv, InventoryStorable *item, Vector2i grid_position);
EntityID try_get_inventory_item_at_position(
    struct EntitySystem *es, Inventory *inv, Vector2i grid_position);

static inline void ensure_valid_item_grid_size(InventoryStorable *item)
{
    ASSERT(item->inventory_grid_size.x <= INVENTORY_GRID_CELL_COUNTS.x);
    ASSERT(item->inventory_grid_size.y <= INVENTORY_GRID_CELL_COUNTS.y);

    // Default to 1x1 item size if the component was zero initialized
    item->inventory_grid_size.x = MAX(1, item->inventory_grid_size.x);
    item->inventory_grid_size.y = MAX(1, item->inventory_grid_size.y);
}

static inline b32 inventory_is_empty(Inventory *inventory)
{
    b32 result = entity_id_is_null(inventory->first_item_in_inventory);
    ASSERT((!result || entity_id_is_null(inventory->last_item_in_inventory))
           && "Either both head and tail should be null, or neither should be");

    return result;
}

#endif //INVENTORY_H
