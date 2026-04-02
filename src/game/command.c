#include "command.h"

#include "base/linear_arena.h"
#include "base/list.h"
#include "base/utils.h"
#include "components/equipment.h"
#include "entity/entity_id.h"
#include "ui/game_ui.h"
#include "world/world.h"

// TODO: rather than performing and then rolling back transaction,
// check beforehand that the transaction passes and then commit it

// TODO: move things related to item transactions to separate file
typedef struct {
    b32 ok;
    EntityID source_item_replaced_with;
} MoveItemToResult;

static MoveItemToResult move_item_to_destination(Entity *item_entity, ItemLocation source,
    ItemLocation destination, EntitySystem *es, GameUI *game_ui, Inventory *player_inventory,
    Equipment *player_equipment, Vector2 player_position)
{
    MoveItemToResult result = {0};

    InventoryStorable *inv_item = es_get_component(item_entity, InventoryStorable);
    Equippable *equippable = es_try_get_component(item_entity, Equippable);

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (destination.kind) {
        case ITEM_LOCATION_WORLD: {
            world_drop_item_from_position(player_position, item_entity);
            result.ok = true;
        } break;

        case ITEM_LOCATION_INVENTORY: {
            // TODO: are these special cases really needed?
            if (source.kind == ITEM_LOCATION_CURSOR) {
                InventoryInsertion insertion = try_place_or_exchange_inventory_item(es,
                    player_inventory, inv_item, destination.inventory_coords);

                result.ok = insertion.ok;
                result.source_item_replaced_with = insertion.exchanged_item;
            } else {
                if (destination.inventory_coords_provided) {
                    result.ok = try_add_item_to_inventory_at(es, player_inventory, inv_item,
                        destination.inventory_coords);
                } else {
                    result.ok = try_add_item_to_inventory(es, player_inventory, inv_item);
                }
            }
        } break;

        case ITEM_LOCATION_CURSOR: {
            result.ok = true;
            result.source_item_replaced_with = game_ui->inventory_menu.item_on_cursor;

            game_ui->inventory_menu.item_on_cursor = es_get_id_of_entity(es, item_entity);
        } break;

        case ITEM_LOCATION_EQUIP_SLOT: {
            if (equippable) {
                // TODO: try to make inventory insertion as simple as this
                EquipResult equip = {0};

                if (destination.equipment_slot_provided) {
                    equip = try_equip_item_in_slot(es, player_equipment, equippable,
                        destination.equipment_slot);
                } else {
                    equip = try_equip_item_in_any_slot(es, player_equipment, equippable);
                }

                result.ok = equip.ok;
                result.source_item_replaced_with = equip.replaced_item;
            }
        } break;

            INVALID_CASE(ITEM_LOCATION_NULL);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    return result;
}

static void move_item_from_source(Entity *item_entity, ItemLocation source,
    ItemLocation destination, MoveItemToResult move_to_result, EntitySystem *es,
    GameUI *game_ui, Inventory *player_inventory, Equipment *player_equipment,
    Vector2 player_position)
{
    ASSERT(move_to_result.ok);

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

        case ITEM_LOCATION_EQUIP_SLOT: {
            unequip_item(es, player_equipment, source.equipment_slot);
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

        ItemLocation new_destination = source;

        if ((destination.kind == ITEM_LOCATION_EQUIP_SLOT)
            && (source.kind == ITEM_LOCATION_INVENTORY)) {
            // NOTE: If we eqiupped from inventory into first suitable equipment slot,
            // when exchanging the items in inventory and equipment slot we don't want to
            // reuse the same inventory coordinates, but instead place in first suitable
            // inventory slot.
            new_destination.inventory_coords_provided = false;
        }

        // NOTE: Exchanging the source item and item originally found in
        // destination could fail, for example if we are equipping a small item
        // from inventory, which causes a larger item to be unequipped, and that
        // item doesn't fit.
        // TODO: handle this
        move_item_to_destination(exchanged_item_entity, item_location_null(), new_destination,
            es, game_ui, player_inventory, player_equipment, player_position);
    }
}

static void execute_item_transaction(MoveItemCommand command, World *world, GameUI *game_ui)
{
    Entity *player = world_get_player_entity(world);
    PhysicsComponent *player_physics = es_get_component(player, PhysicsComponent);
    Inventory *player_inventory = es_get_component(player, Inventory);
    Equipment *player_equipment = es_get_component(player, Equipment);

    Entity *item_entity = es_try_get_entity(&world->entity_system, command.item_id);

    if (item_entity) {
        MoveItemToResult move_to_result = move_item_to_destination(item_entity, command.source,
            command.destination, &world->entity_system, game_ui, player_inventory,
            player_equipment, player_physics->position);

        if (move_to_result.ok) {
            move_item_from_source(item_entity, command.source, command.destination,
                move_to_result, &world->entity_system, game_ui, player_inventory,
                player_equipment, player_physics->position);
        }
    }
}

void push_command(CommandQueue *commands, Command command, LinearArena *arena)
{
    Command *command_node = la_allocate_item(arena, Command);
    *command_node = command;

    sl_list_push_back(commands, command_node);
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

        sl_list_pop(commands);
        current = list_head(commands);
    }
}
