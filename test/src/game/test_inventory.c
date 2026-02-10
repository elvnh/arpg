#include "entity/entity_id.h"
#include "test_macros.h"
#include "testing_utils.h"
#include "entity/entity_system.h"
#include "components/component.h"

/*
  TODO:
  - create_inventory_holder
  - REQUIRE_ENTITY_IDS_EQ
 */

typedef struct {
    Entity *entity;
    InventoryStorable *item;
} ItemEntity;

static ItemEntity create_item_entity(EntitySystem *es)
{
    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    InventoryStorable *item = es_add_component(e.entity, InventoryStorable);

    ItemEntity result = {e.entity, item};

    return result;
}

TEST_CASE(inventory_append_basic) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    REQUIRE(inventory_is_empty(inv));

    ItemEntity item_entity = create_item_entity(es);
    REQUIRE(!inventory_contains_item(es, inv, item_entity.item));
    REQUIRE(entity_id_is_null(inv->first_item_in_inventory));
    REQUIRE(entity_id_is_null(inv->last_item_in_inventory));

    append_item_to_inventory(es, inv, item_entity.item);

    REQUIRE(!inventory_is_empty(inv));
    REQUIRE(inventory_contains_item(es, inv, item_entity.item));

    free_entity_system(es);
}

TEST_CASE(inventory_append_empty) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity item_entity = create_item_entity(es);
    append_item_to_inventory(es, inv, item_entity.item);

    REQUIRE(entity_id_equal(inv->first_item_in_inventory, item_entity.entity->id));
    REQUIRE(entity_id_equal(inv->last_item_in_inventory, item_entity.entity->id));

    free_entity_system(es);
}

TEST_CASE(inventory_append_should_remove_position_component) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity item_entity = create_item_entity(es);
    es_add_component(item_entity.entity, PhysicsComponent);
    append_item_to_inventory(es, inv, item_entity.item);

    REQUIRE(!es_has_component(item_entity.entity, PhysicsComponent));

    free_entity_system(es);
}

TEST_CASE(inventory_append_when_one_element) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity first_item = create_item_entity(es);
    append_item_to_inventory(es, inv, first_item.item);

    ItemEntity last_item = create_item_entity(es);
    append_item_to_inventory(es, inv, last_item.item);

    REQUIRE(entity_id_equal(inv->first_item_in_inventory, first_item.entity->id));
    REQUIRE(entity_id_equal(inv->last_item_in_inventory, last_item.entity->id));

    REQUIRE(inventory_contains_item(es, inv, first_item.item));
    REQUIRE(inventory_contains_item(es, inv, last_item.item));

    REQUIRE(entity_id_equal(first_item.item->next_item_in_inventory, last_item.entity->id));
    REQUIRE(entity_id_equal(last_item.item->prev_item_in_inventory, first_item.entity->id));

    free_entity_system(es);
}

TEST_CASE(inventory_append_when_two_elements) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity first_item = create_item_entity(es);
    append_item_to_inventory(es, inv, first_item.item);

    ItemEntity middle_item = create_item_entity(es);
    append_item_to_inventory(es, inv, middle_item.item);

    ItemEntity last_item = create_item_entity(es);
    append_item_to_inventory(es, inv, last_item.item);

    REQUIRE(entity_id_equal(inv->first_item_in_inventory, first_item.entity->id));
    REQUIRE(entity_id_equal(inv->last_item_in_inventory, last_item.entity->id));

    REQUIRE(inventory_contains_item(es, inv, first_item.item));
    REQUIRE(inventory_contains_item(es, inv, middle_item.item));
    REQUIRE(inventory_contains_item(es, inv, last_item.item));

    free_entity_system(es);
}

TEST_CASE(inventory_insert_after_when_one_element) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity first_item = create_item_entity(es);
    append_item_to_inventory(es, inv, first_item.item);

    ItemEntity inserted_item = create_item_entity(es);
    insert_item_in_inventory(es, inv, inserted_item.item, first_item.item);

    REQUIRE(entity_id_equal(inv->first_item_in_inventory, first_item.entity->id));
    REQUIRE(entity_id_equal(inv->last_item_in_inventory, inserted_item.entity->id));

    REQUIRE(inventory_contains_item(es, inv, first_item.item));
    REQUIRE(inventory_contains_item(es, inv, inserted_item.item));

    free_entity_system(es);
}

TEST_CASE(inventory_insert_after_first_when_two_elements) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity first_item = create_item_entity(es);
    append_item_to_inventory(es, inv, first_item.item);

    ItemEntity last_item = create_item_entity(es);
    append_item_to_inventory(es, inv, last_item.item);

    ItemEntity inserted_item = create_item_entity(es);
    insert_item_in_inventory(es, inv, inserted_item.item, first_item.item);

    REQUIRE(entity_id_equal(inv->first_item_in_inventory, first_item.entity->id));
    REQUIRE(entity_id_equal(inv->last_item_in_inventory, last_item.entity->id));

    REQUIRE(inventory_contains_item(es, inv, first_item.item));
    REQUIRE(inventory_contains_item(es, inv, inserted_item.item));
    REQUIRE(inventory_contains_item(es, inv, last_item.item));

    free_entity_system(es);
}

TEST_CASE(inventory_insert_after_last_when_two_elements) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity first_item = create_item_entity(es);
    append_item_to_inventory(es, inv, first_item.item);

    ItemEntity last_item = create_item_entity(es);
    append_item_to_inventory(es, inv, last_item.item);

    ItemEntity inserted_item = create_item_entity(es);
    insert_item_in_inventory(es, inv, inserted_item.item, last_item.item);

    REQUIRE(entity_id_equal(inv->first_item_in_inventory, first_item.entity->id));
    REQUIRE(entity_id_equal(inv->last_item_in_inventory, inserted_item.entity->id));

    REQUIRE(inventory_contains_item(es, inv, first_item.item));
    REQUIRE(inventory_contains_item(es, inv, inserted_item.item));
    REQUIRE(inventory_contains_item(es, inv, last_item.item));

    free_entity_system(es);
}

TEST_CASE(inventory_insert_after_middle_when_three_elements) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity first_item = create_item_entity(es);
    append_item_to_inventory(es, inv, first_item.item);

    ItemEntity middle_item = create_item_entity(es);
    append_item_to_inventory(es, inv, middle_item.item);

    ItemEntity last_item = create_item_entity(es);
    append_item_to_inventory(es, inv, last_item.item);

    ItemEntity inserted_item = create_item_entity(es);
    insert_item_in_inventory(es, inv, inserted_item.item, middle_item.item);

    REQUIRE(entity_id_equal(inv->first_item_in_inventory, first_item.entity->id));
    REQUIRE(entity_id_equal(inv->last_item_in_inventory, last_item.entity->id));

    REQUIRE(inventory_contains_item(es, inv, first_item.item));
    REQUIRE(inventory_contains_item(es, inv, inserted_item.item));
    REQUIRE(inventory_contains_item(es, inv, middle_item.item));
    REQUIRE(inventory_contains_item(es, inv, last_item.item));

    free_entity_system(es);
}

TEST_CASE(inventory_insert_should_remove_position_component) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity first_item = create_item_entity(es);
    append_item_to_inventory(es, inv, first_item.item);

    ItemEntity inserted_item = create_item_entity(es);
    es_add_component(inserted_item.entity, PhysicsComponent);

    insert_item_in_inventory(es, inv, inserted_item.item, first_item.item);

    REQUIRE(!es_has_component(inserted_item.entity, PhysicsComponent));

    free_entity_system(es);
}

TEST_CASE(inventory_remove_when_one_item) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity item = create_item_entity(es);
    append_item_to_inventory(es, inv, item.item);

    remove_item_from_inventory(es, inv, item.item);

    REQUIRE(inventory_is_empty(inv));
    REQUIRE(!inventory_contains_item(es, inv, item.item));

    free_entity_system(es);
}

TEST_CASE(inventory_remove_first_when_two_items) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity item1 = create_item_entity(es);
    ItemEntity item2 = create_item_entity(es);
    append_item_to_inventory(es, inv, item1.item);
    append_item_to_inventory(es, inv, item2.item);

    remove_item_from_inventory(es, inv, item1.item);

    REQUIRE(!inventory_is_empty(inv));
    REQUIRE(!inventory_contains_item(es, inv, item1.item));
    REQUIRE(inventory_contains_item(es, inv, item2.item));

    REQUIRE(entity_id_is_null(item2.item->next_item_in_inventory));

    free_entity_system(es);
}

TEST_CASE(inventory_remove_last_when_two_items) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity item1 = create_item_entity(es);
    ItemEntity item2 = create_item_entity(es);
    append_item_to_inventory(es, inv, item1.item);
    append_item_to_inventory(es, inv, item2.item);

    remove_item_from_inventory(es, inv, item2.item);

    REQUIRE(inventory_contains_item(es, inv, item1.item));
    REQUIRE(!inventory_contains_item(es, inv, item2.item));

    REQUIRE(entity_id_is_null(item1.item->next_item_in_inventory));

    free_entity_system(es);
}

TEST_CASE(inventory_remove_first_when_three_items) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity item1 = create_item_entity(es);
    ItemEntity item2 = create_item_entity(es);
    ItemEntity item3 = create_item_entity(es);

    append_item_to_inventory(es, inv, item1.item);
    append_item_to_inventory(es, inv, item2.item);
    append_item_to_inventory(es, inv, item3.item);

    remove_item_from_inventory(es, inv, item1.item);

    REQUIRE(!inventory_contains_item(es, inv, item1.item));

    REQUIRE(inventory_contains_item(es, inv, item2.item));
    REQUIRE(inventory_contains_item(es, inv, item3.item));

    REQUIRE(entity_id_is_null(item2.item->prev_item_in_inventory));
    REQUIRE(entity_id_equal(item2.item->next_item_in_inventory, item3.entity->id));

    REQUIRE(entity_id_is_null(item3.item->next_item_in_inventory));
    REQUIRE(entity_id_equal(item3.item->prev_item_in_inventory, item2.entity->id));

    free_entity_system(es);
}

TEST_CASE(inventory_remove_middle_when_three_items) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity item1 = create_item_entity(es);
    ItemEntity item2 = create_item_entity(es);
    ItemEntity item3 = create_item_entity(es);

    append_item_to_inventory(es, inv, item1.item);
    append_item_to_inventory(es, inv, item2.item);
    append_item_to_inventory(es, inv, item3.item);

    remove_item_from_inventory(es, inv, item2.item);

    REQUIRE(inventory_contains_item(es, inv, item1.item));
    REQUIRE(!inventory_contains_item(es, inv, item2.item));
    REQUIRE(inventory_contains_item(es, inv, item3.item));

    REQUIRE(entity_id_is_null(item1.item->prev_item_in_inventory));
    REQUIRE(entity_id_equal(item1.item->next_item_in_inventory, item3.entity->id));

    REQUIRE(entity_id_is_null(item3.item->next_item_in_inventory));
    REQUIRE(entity_id_equal(item3.item->prev_item_in_inventory, item1.entity->id));

    free_entity_system(es);
}

TEST_CASE(inventory_remove_last_when_three_items) {
    EntitySystem *es = allocate_entity_system();

    EntityWithID e = es_create_entity(es, FACTION_NEUTRAL);
    Inventory *inv = es_add_component(e.entity, Inventory);

    ItemEntity item1 = create_item_entity(es);
    ItemEntity item2 = create_item_entity(es);
    ItemEntity item3 = create_item_entity(es);

    append_item_to_inventory(es, inv, item1.item);
    append_item_to_inventory(es, inv, item2.item);
    append_item_to_inventory(es, inv, item3.item);

    remove_item_from_inventory(es, inv, item3.item);

    REQUIRE(inventory_contains_item(es, inv, item1.item));
    REQUIRE(inventory_contains_item(es, inv, item2.item));
    REQUIRE(!inventory_contains_item(es, inv, item3.item));

    REQUIRE(entity_id_is_null(item1.item->prev_item_in_inventory));
    REQUIRE(entity_id_equal(item1.item->next_item_in_inventory, item2.entity->id));

    REQUIRE(entity_id_is_null(item2.item->next_item_in_inventory));
    REQUIRE(entity_id_equal(item2.item->prev_item_in_inventory, item1.entity->id));

    free_entity_system(es);
}
