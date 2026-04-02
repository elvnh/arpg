#ifndef ITEM_TRANSACTION_H
#define ITEM_TRANSACTION_H

#include "base/typedefs.h"
#include "components/equipment.h"

struct GameUI;
struct World;

typedef enum {
    ITEM_LOCATION_NULL,
    ITEM_LOCATION_WORLD,
    ITEM_LOCATION_INVENTORY,
    ITEM_LOCATION_CURSOR,
    ITEM_LOCATION_EQUIP_SLOT,
} ItemLocationKind;

typedef struct {
    ItemLocationKind kind;

    // TODO: put these in a union
    Vector2i inventory_coords;
    b32 inventory_coords_provided;

    EquipmentSlot equipment_slot;
    b32 equipment_slot_provided;
} ItemLocation;

typedef struct {
    EntityID item_id;
    ItemLocation source;
    ItemLocation destination;
} ItemTransaction;

void execute_item_transaction(ItemTransaction transaction, struct World *world,
    struct GameUI *game_ui);

static inline ItemTransaction item_transaction(EntityID item_id, ItemLocation source,
    ItemLocation destination)
{
    ItemTransaction result = {0};
    result.item_id = item_id;
    result.source = source;
    result.destination = destination;

    return result;
}

static inline ItemLocation item_location_world(void)
{
    ItemLocation result = {0};
    result.kind = ITEM_LOCATION_WORLD;

    return result;
}

static inline ItemLocation item_location_inventory(Vector2i inventory_coords)
{
    ItemLocation result = {0};
    result.kind = ITEM_LOCATION_INVENTORY;
    result.inventory_coords = inventory_coords;
    result.inventory_coords_provided = true;

    return result;
}

static inline ItemLocation item_location_any_inventory_slot(void)
{
    ItemLocation result = {0};
    result.kind = ITEM_LOCATION_INVENTORY;

    return result;
}

static inline ItemLocation item_location_cursor(void)
{
    ItemLocation result = {0};
    result.kind = ITEM_LOCATION_CURSOR;

    return result;
}

static inline ItemLocation item_location_equipment_slot(EquipmentSlot slot)
{
    ItemLocation result = {0};
    result.kind = ITEM_LOCATION_EQUIP_SLOT;
    result.equipment_slot = slot;
    result.equipment_slot_provided = true;

    return result;
}

static inline ItemLocation item_location_any_equipment_slot(void)
{
    ItemLocation result = {0};
    result.kind = ITEM_LOCATION_EQUIP_SLOT;

    return result;
}

static inline ItemLocation item_location_null(void)
{
    ItemLocation result = {0};

    return result;
}

#endif // ITEM_TRANSACTION_H
