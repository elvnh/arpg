#include "inventory.h"

#include "base/rectangle.h"
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
    }

    return result;
}

// TODO: this isn't needed anymore since only append to list is allowed
static void insert_item_in_inventory(EntitySystem *es, Inventory *inv,
    InventoryStorable *item_to_add, InventoryStorable *insert_after)
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

    InventoryStorable *next_item =
        get_inventory_item(es, insert_after->next_item_in_inventory);

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

void append_item_to_inventory_list(
    EntitySystem *es, Inventory *inv, InventoryStorable *item_to_add)
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

// TODO: this doesn't need to count above 1
typedef struct {
    s32 collision_count;
    EntityID first_collision_item;
} InventoryCollision;

static InventoryCollision count_inventory_item_collisions(
    EntitySystem *es, Inventory *inv, InventoryStorable *item, Vector2i grid_pos)
{
    InventoryCollision result = {0};

    Rectangle self_rect = {
        v2i_to_v2(grid_pos),
        v2i_to_v2(item->inventory_grid_size),
    };

    Entity *item_entity = es_get_component_owner(es, item, InventoryStorable);
    EntityID curr_id = inv->first_item_in_inventory;

    while (!entity_id_is_null(curr_id)) {
        ASSERT(!entity_id_equal(curr_id, es_get_id_of_entity(es, item_entity))
               && "Should not be called if item is already in inventory");

        Entity *other_entity = es_get_entity(es, curr_id);
        InventoryStorable *other_item = es_get_component(other_entity, InventoryStorable);

        Rectangle other_rect = {
            v2i_to_v2(other_item->inventory_grid_position),
            v2i_to_v2(other_item->inventory_grid_size),
        };

        if (rect_intersects(self_rect, other_rect)) {
            ++result.collision_count;

            if (result.collision_count == 1) {
                result.first_collision_item = curr_id;
            }
        }

        curr_id = other_item->next_item_in_inventory;
    }

    return result;
}

InventoryInsertion can_add_item_to_inventory_at(EntitySystem *es, Inventory *inv,
    InventoryStorable *item, Vector2i grid_position)
{
    InventoryInsertion result = {0};

    if (item_is_in_bounds_of_inventory_grid(grid_position, item->inventory_grid_size)) {
        InventoryCollision collision =
            count_inventory_item_collisions(es, inv, item, grid_position);

        result.ok = collision.collision_count == 0;
        result.inventory_coords = grid_position;
    }

    return result;
}

static void add_item_to_inventory_at(EntitySystem *es, Inventory *inv, InventoryStorable *item,
    Vector2i grid_position)
{
    ASSERT(!inventory_contains_item(es, inv, item));
    ASSERT(can_add_item_to_inventory_at(es, inv, item, grid_position).ok);

    ensure_valid_item_grid_size(item);

    ASSERT(grid_position.x >= 0);
    ASSERT((grid_position.x + item->inventory_grid_size.x) <= INVENTORY_GRID_CELL_COUNTS.x);
    ASSERT(grid_position.y >= 0);
    ASSERT((grid_position.y + item->inventory_grid_size.y) <= INVENTORY_GRID_CELL_COUNTS.y);

    item->inventory_grid_position = grid_position;
    append_item_to_inventory_list(es, inv, item);
}

InventoryInsertion can_add_item_to_inventory(struct EntitySystem *es, Inventory *inv,
    InventoryStorable *item)
{
    // TODO: this function is horribly inefficient, do something more clever
    InventoryInsertion result = {0};

    s32 max_x = INVENTORY_GRID_CELL_COUNTS.x - item->inventory_grid_size.x;
    s32 max_y = INVENTORY_GRID_CELL_COUNTS.y - item->inventory_grid_size.y;

    for (s32 x = 0; x <= max_x && !result.ok; ++x) {
        for (s32 y = 0; y <= max_y && !result.ok; ++y) {
            Vector2i coords = {x, y};

            result = can_add_item_to_inventory_at(es, inv, item, coords);

            if (result.ok) {
                break;
            }
        }
    }

    return result;
}

InventoryInsertion try_add_item_to_inventory(struct EntitySystem *es, Inventory *inv,
    InventoryStorable *item)
{
    InventoryInsertion result = can_add_item_to_inventory(es, inv, item);

    if (result.ok) {
        add_item_to_inventory_at(es, inv, item, result.inventory_coords);
    }

    return result;
}

void remove_item_from_inventory(
    EntitySystem *es, Inventory *inv, InventoryStorable *item_to_remove)
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

    item_to_remove->prev_item_in_inventory = NULL_ENTITY_ID;
    item_to_remove->next_item_in_inventory = NULL_ENTITY_ID;
}

void drop_item_from_inventory_on_ground(
    EntitySystem *es, Inventory *inv, InventoryStorable *item)
{
    ASSERT(inventory_contains_item(es, inv, item));
    Entity *owner = es_get_component_owner(es, inv, Inventory);
    PhysicsComponent *owner_physics = es_get_component(owner, PhysicsComponent);

    Entity *item_entity = es_get_component_owner(es, item, InventoryStorable);
    ASSERT(!es_has_component(item_entity, PhysicsComponent));

    remove_item_from_inventory(es, inv, item);

    world_drop_item_from_position(owner_physics->position, item_entity);
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

EntityID try_get_inventory_item_at_position(
    struct EntitySystem *es, Inventory *inv, Vector2i grid_position)
{
    EntityID curr_id = inv->first_item_in_inventory;

    while (!entity_id_is_null(curr_id)) {
        Entity *curr_entity = es_get_entity(es, curr_id);
        InventoryStorable *curr_item = es_get_component(curr_entity, InventoryStorable);

        Vector2i curr_pos = curr_item->inventory_grid_position;
        Vector2i curr_size = curr_item->inventory_grid_size;

        if ((grid_position.x >= curr_pos.x) && (grid_position.x < curr_pos.x + curr_size.x)
            && (grid_position.y >= curr_pos.y)
            && (grid_position.y < curr_pos.y + curr_size.y)) {
            break;
        }

        curr_id = curr_item->next_item_in_inventory;
    }

    return curr_id;
}

b32 item_is_in_bounds_of_inventory_grid(Vector2i item_grid_coords, Vector2i item_grid_size)
{
    s32 item_right = item_grid_coords.x + item_grid_size.x;
    s32 item_bottom = item_grid_coords.y + item_grid_size.y;

    b32 result = (item_grid_coords.x >= 0) && (item_right <= INVENTORY_GRID_CELL_COUNTS.x)
                 && (item_grid_coords.y >= 0) && (item_bottom <= INVENTORY_GRID_CELL_COUNTS.y);

    return result;
}

b32 item_is_completely_outside_inventory_grid(Vector2i item_grid_coords,
    Vector2i item_grid_size)
{
    s32 item_right = item_grid_coords.x + item_grid_size.x;
    s32 item_bottom = item_grid_coords.y + item_grid_size.y;

    b32 result = ((item_right < 0) || (item_grid_coords.x > INVENTORY_GRID_CELL_COUNTS.x))
                 || ((item_bottom < 0) || (item_grid_coords.y > INVENTORY_GRID_CELL_COUNTS.y));

    return result;
}

b32 cell_is_in_bounds_of_inventory_grid(Vector2i cell_coords)
{
    b32 result = item_is_in_bounds_of_inventory_grid(cell_coords, v2i(1, 1));

    return result;
}

InventoryInsertion can_place_or_exchange_inventory_item_at(struct EntitySystem *es,
    Inventory *inventory, InventoryStorable *item, Vector2i grid_pos)
{
    InventoryInsertion result = {0};
    b32 in_bounds = item_is_in_bounds_of_inventory_grid(item->inventory_grid_position,
        item->inventory_grid_size);

    if (in_bounds) {
        InventoryCollision collision =
            count_inventory_item_collisions(es, inventory, item, grid_pos);

        result.ok = collision.collision_count <= 1;

        if (collision.collision_count == 1) {
            // Item only collides with one other, we can exchange it
            ASSERT(!entity_id_is_null(collision.first_collision_item));

            result.exchanged_item = collision.first_collision_item;
        }
    }

    return result;
}

InventoryInsertion try_place_or_exchange_inventory_item(EntitySystem *es, Inventory *inventory,
    InventoryStorable *item, Vector2i grid_pos)
{
    InventoryInsertion result =
        can_place_or_exchange_inventory_item_at(es, inventory, item, grid_pos);

    if (result.ok) {
        Entity *exchanged_entity = es_try_get_entity(es, result.exchanged_item);

        if (exchanged_entity) {
            InventoryStorable *exchanged_item =
                es_get_component(exchanged_entity, InventoryStorable);
            remove_item_from_inventory(es, inventory, exchanged_item);
        }

        add_item_to_inventory_at(es, inventory, item, grid_pos);
    }

    return result;
}
