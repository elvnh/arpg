#include "item_transaction.h"

#include "entity/entity_system.h"
#include "ui/game_ui.h"
#include "world/world.h"

typedef struct {
    b32 ok;
    EntityID source_item_replaced_with;
} MoveItemToResult;

static MoveItemToResult can_move_item_to_destination(Entity *item_entity, ItemLocation source,
    ItemLocation destination, EntitySystem *es, GameUI *game_ui, Inventory *player_inventory,
    Equipment *player_equipment)
{
    MoveItemToResult result = {0};

    InventoryStorable *inv_item = es_get_component(item_entity, InventoryStorable);
    Equippable *equippable = es_try_get_component(item_entity, Equippable);

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (destination.kind) {
        case ITEM_LOCATION_WORLD: {
            result.ok = true;
        } break;

        case ITEM_LOCATION_INVENTORY: {
            InventoryInsertion insertion = {0};

            if (destination.inventory_coords_provided) {
                insertion = can_place_or_exchange_inventory_item_at(es, player_inventory,
                    inv_item, destination.inventory_coords);

                if (!insertion.ok && (source.kind != ITEM_LOCATION_CURSOR)) {
                    // NOTE: As long as we're not moving item from cursor, we want to
                    // first try inserting at exact position and then at the first available
                    // position
                    insertion = can_add_item_to_inventory(es, player_inventory, inv_item);
                }
            } else {
                insertion = can_add_item_to_inventory(es, player_inventory, inv_item);
            }

            result.ok = insertion.ok;
            result.source_item_replaced_with = insertion.exchanged_item;

        } break;

        case ITEM_LOCATION_CURSOR: {
            result.ok = true;
            result.source_item_replaced_with = game_ui->inventory_menu.item_on_cursor;
        } break;

        case ITEM_LOCATION_EQUIP_SLOT: {
            if (equippable) {
                EquipResult equip = {0};

                if (destination.equipment_slot_provided) {
                    equip = can_equip_item_in_slot(player_equipment, equippable,
                        destination.equipment_slot);
                } else {
                    equip = can_equip_item_in_any_slot(player_equipment, equippable);
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

static b32 can_move_item_from_source(ItemLocation source, MoveItemToResult move_to_result,
    EntitySystem *es, GameUI *game_ui, Inventory *player_inventory,
    Equipment *player_equipment)
{
    ASSERT(move_to_result.ok);

    // NOTE: Moving item from a location can only fail if the exchange with the
    // item currently at that location fails
    b32 result = true;

    if (!entity_id_is_null(move_to_result.source_item_replaced_with)) {
        // If we exchanged the source item and the item that was currently
        // occupying destination, we move that item to the source location
        Entity *exchanged_item_entity =
            es_get_entity(es, move_to_result.source_item_replaced_with);

        // NOTE: Exchanging the source item and item originally found in
        // destination could fail, for example if we are equipping a small item
        // from inventory, which causes a larger item to be unequipped, and that
        // item doesn't fit in inventory.
        MoveItemToResult exchange = can_move_item_to_destination(exchanged_item_entity,
            item_location_null(), source, es, game_ui, player_inventory, player_equipment);

        result = exchange.ok;
    }

    return result;
}

static void move_item_to_destination(Entity *item_entity, ItemLocation destination,
    EntitySystem *es, GameUI *game_ui, Inventory *player_inventory,
    Equipment *player_equipment, Vector2 player_position)
{
    InventoryStorable *inv_item = es_get_component(item_entity, InventoryStorable);
    Equippable *equippable = es_try_get_component(item_entity, Equippable);

    ensure_valid_item_grid_size(inv_item);

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (destination.kind) {
        case ITEM_LOCATION_WORLD: {
            world_drop_item_from_position(player_position, item_entity);
        } break;

        case ITEM_LOCATION_INVENTORY: {
            if (destination.inventory_coords_provided) {
                place_or_exchange_inventory_item_at(es, player_inventory, inv_item,
                    destination.inventory_coords);
            } else {
                add_item_to_inventory(es, player_inventory, inv_item);
            }
        } break;

        case ITEM_LOCATION_CURSOR: {
            game_ui->inventory_menu.item_on_cursor = es_get_id_of_entity(es, item_entity);
        } break;

        case ITEM_LOCATION_EQUIP_SLOT: {
            if (equippable) {
                // TODO: try to make inventory insertion as simple as this
                EquipResult equip = {0};

                if (destination.equipment_slot_provided) {
                    equip = equip_item_in_slot(es, player_equipment, equippable,
                        destination.equipment_slot);
                } else {
                    equip = equip_item_in_any_slot(es, player_equipment, equippable);
                }

                ASSERT(equip.ok);
            }
        } break;

            INVALID_CASE(ITEM_LOCATION_NULL);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;
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
            ASSERT(entity_id_equal(item_entity->id, game_ui->inventory_menu.item_on_cursor));

            game_ui->inventory_menu.item_on_cursor = move_to_result.source_item_replaced_with;
        } break;

        case ITEM_LOCATION_EQUIP_SLOT: {
            unequip_item_in_slot(player_equipment, source.equipment_slot);
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

        move_item_to_destination(exchanged_item_entity, new_destination, es, game_ui,
            player_inventory, player_equipment, player_position);
    }
}

void execute_item_transaction(ItemTransaction transaction, World *world, GameUI *game_ui)
{
    Entity *player = world_get_player_entity(world);
    PhysicsComponent *player_physics = es_get_component(player, PhysicsComponent);
    Inventory *player_inventory = es_get_component(player, Inventory);
    Equipment *player_equipment = es_get_component(player, Equipment);

    Entity *item_entity = es_try_get_entity(&world->entity_system, transaction.item_id);

    if (item_entity) {
        MoveItemToResult move_to_result = can_move_item_to_destination(item_entity,
            transaction.source, transaction.destination, &world->entity_system, game_ui,
            player_inventory, player_equipment);

        b32 success = move_to_result.ok
                      && can_move_item_from_source(transaction.source, move_to_result,
                          &world->entity_system, game_ui, player_inventory, player_equipment);

        if (success) {
            move_item_to_destination(item_entity, transaction.destination,
                &world->entity_system, game_ui, player_inventory, player_equipment,
                player_physics->position);

            move_item_from_source(item_entity, transaction.source, transaction.destination,
                move_to_result, &world->entity_system, game_ui, player_inventory,
                player_equipment, player_physics->position);
        }
    }
}
