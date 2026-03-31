#include "command.h"

#include "base/linear_arena.h"
#include "base/list.h"
#include "ui/game_ui.h"
#include "world/world.h"

typedef struct {
    b32 ok;
    EntityID source_item_replaced_with;
} MoveItemToResult;

MoveItemToResult move_item_to_destination(Entity *item_entity, ItemLocation source,
    ItemLocation destination, EntitySystem *es, GameUI *game_ui, Inventory *player_inventory,
    Vector2 player_position)
{
    MoveItemToResult result = {0};

    InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (destination.kind) {
        case ITEM_LOCATION_WORLD: {
            world_drop_item_from_position(player_position, item_entity);
            result.ok = true;
        } break;

        case ITEM_LOCATION_INVENTORY: {
            if (source.kind == ITEM_LOCATION_CURSOR) {
                InventoryInsertion insertion = try_place_or_exchange_inventory_item(es,
                    player_inventory, item, destination.inventory_coords);

                result.ok = insertion.ok;
                result.source_item_replaced_with = insertion.exchanged_item;
            } else {
                if (destination.inventory_coords_provided) {
                    result.ok = try_add_item_to_inventory_at(es, player_inventory, item,
                        destination.inventory_coords);
                } else {
                    result.ok = try_add_item_to_inventory(es, player_inventory, item);
                }
            }
        } break;

        case ITEM_LOCATION_CURSOR: {
            result.ok = true;
            result.source_item_replaced_with = game_ui->inventory_menu.item_on_cursor;
            game_ui->inventory_menu.item_on_cursor = es_get_id_of_entity(es, item_entity);
        } break;

            INVALID_CASE(ITEM_LOCATION_NULL);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    return result;
}

static void move_item_from_source(Entity *item_entity, ItemLocation source,
    MoveItemToResult move_to_result, EntitySystem *es, GameUI *game_ui,
    Inventory *player_inventory, Vector2 player_position)
{
    InventoryStorable *item = es_get_component(item_entity, InventoryStorable);

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (source.kind) {
        case ITEM_LOCATION_WORLD: {
            es_remove_component(item_entity, PhysicsComponent);
        } break;

        case ITEM_LOCATION_INVENTORY: {
            remove_item_from_inventory(es, player_inventory, item);
        } break;

        case ITEM_LOCATION_CURSOR: {
            game_ui->inventory_menu.item_on_cursor = move_to_result.source_item_replaced_with;

        } break;

            INVALID_CASE(ITEM_LOCATION_NULL);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    if (!entity_id_is_null(move_to_result.source_item_replaced_with)) {
        // If we exchanged the source item and the item that was currently
        // occupying destination, we move that item to the source location
        Entity *exchanged_item_entity =
            es_get_entity(es, move_to_result.source_item_replaced_with);

        move_item_to_destination(exchanged_item_entity, item_location_null(), source, es,
            game_ui, player_inventory, player_position);
    }
}

static void execute_item_transaction(MoveItemCommand command, World *world, GameUI *game_ui)
{
    Entity *player = world_get_player_entity(world);
    PhysicsComponent *player_physics = es_get_component(player, PhysicsComponent);
    Inventory *player_inventory = es_get_component(player, Inventory);

    Entity *item_entity = es_get_entity(&world->entity_system, command.item_id);

    MoveItemToResult move_to_result =
        move_item_to_destination(item_entity, command.source, command.destination,
            &world->entity_system, game_ui, player_inventory, player_physics->position);

    if (move_to_result.ok) {
        move_item_from_source(item_entity, command.source, move_to_result,
            &world->entity_system, game_ui, player_inventory, player_physics->position);
    }
}

void push_command(CommandQueue *commands, Command command, LinearArena *arena)
{
    Command *command_node = la_allocate_item(arena, Command);
    *command_node = command;

    list_push_back(commands, command_node);
}

void execute_command_queue(CommandQueue *commands, World *world, GameUI *game_ui)
{
    Command *current = list_head(commands);

    // TODO: perform more validation in this function
    while (current) {
        BEGIN_EXHAUSTIVE_SWITCH;
        switch (current->kind) {
            case COMMAND_WALK: {
                MoveCommand *cmd = &current->as.move;

                Entity *player = world_get_player_entity(world);
                PhysicsComponent *physics = es_get_component(player, PhysicsComponent);
                entity_try_transition_to_state(world, player, physics,
                    state_walking(cmd->direction));
            } break;

            case COMMAND_ATTACK: {
                AttackCommand *cmd = &current->as.attack;

                Entity *player = world_get_player_entity(world);
                SpellCasterComponent *spellcaster =
                    es_get_component(player, SpellCasterComponent);
                SpellID selected_spell = get_spell_at_spellbook_index(spellcaster,
                    game_ui->selected_spellbook_index);

                try_cast_spell(world, selected_spell, player, cmd->target);
            } break;

            case COMMAND_MOVE_ITEM: {
                MoveItemCommand *cmd = &current->as.move_item;

                execute_item_transaction(*cmd, world, game_ui);
            } break;
        }
        END_EXHAUSTIVE_SWITCH;

        list_pop_head(commands);
        current = list_head(commands);
    }
}
