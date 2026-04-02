#ifndef COMMAND_H
#define COMMAND_H

#include "base/linear_arena.h"
#include "base/vector.h"
#include "components/equipment.h"
#include "game/entity/entity_id.h"

/* A Command represents an action that the player wishes to perform, such as
 * attacking in a certain direction. Other parts of the game merely need to
 * create such actions and push them to a queue, and this subsystem will take
 * care of validating and executing them.
 */

struct World;
struct GameUI;

typedef enum {
    COMMAND_ATTACK,
    COMMAND_WALK,
    COMMAND_MOVE_ITEM,
} CommandKind;

typedef struct {
    Vector2 target;
} AttackCommand;

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
} MoveItemCommand; // TODO: rename to itemtransaction

typedef struct {
    Vector2 direction;
} MoveCommand;

typedef struct Command {
    CommandKind kind;

    union {
        AttackCommand attack;
        MoveCommand move;
        MoveItemCommand move_item;
    } as;

    struct Command *next;
} Command;

typedef struct {
    Command *head;
    Command *tail;
} CommandQueue;

void push_command(CommandQueue *commands, Command command, LinearArena *arena);
void execute_command_queue(CommandQueue *commands, struct World *world,
    struct GameUI *game_ui);

static inline Command attack_command(Vector2 target)
{
    Command result = {0};
    result.kind = COMMAND_ATTACK;
    result.as.attack = (AttackCommand){.target = target};

    return result;
}

// TODO: rename to transfer item
static inline Command move_item_command(EntityID item_id, ItemLocation source,
    ItemLocation destination)
{
    Command result = {0};
    result.kind = COMMAND_MOVE_ITEM;
    result.as.move_item =
        (MoveItemCommand){.item_id = item_id, .source = source, .destination = destination};

    return result;
}

static inline Command move_command(Vector2 direction)
{
    Command result = {0};
    result.kind = COMMAND_WALK;
    result.as.move = (MoveCommand){.direction = direction};

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

#endif // COMMAND_H
