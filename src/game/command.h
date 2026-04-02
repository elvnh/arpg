#ifndef COMMAND_H
#define COMMAND_H

#include "base/linear_arena.h"
#include "base/vector.h"
#include "components/equipment.h"
#include "game/entity/entity_id.h"
#include "item_transaction.h"

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
    COMMAND_ITEM_TRANSACTION,
} CommandKind;

typedef struct {
    Vector2 target;
} AttackCommand;

typedef struct {
    ItemTransaction transaction;
} ItemTransactionCommand;

typedef struct {
    Vector2 direction;
} MoveCommand;

typedef struct Command {
    CommandKind kind;

    union {
        AttackCommand attack;
        MoveCommand move;
        ItemTransactionCommand item_transaction;
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

static inline Command item_transaction_command(EntityID item_id, ItemLocation source,
    ItemLocation destination)
{
    Command result = {0};

    result.kind = COMMAND_ITEM_TRANSACTION;
    result.as.item_transaction.transaction = item_transaction(item_id, source, destination);

    return result;
}

static inline Command move_command(Vector2 direction)
{
    Command result = {0};
    result.kind = COMMAND_WALK;
    result.as.move = (MoveCommand){.direction = direction};

    return result;
}

#endif // COMMAND_H
