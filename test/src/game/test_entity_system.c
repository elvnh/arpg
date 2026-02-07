#include "components/component.h"
#include "entity/entity_faction.h"
#include "entity/entity_id.h"
#include "test_macros.h"
#include "game/entity/entity_system.h"

#include <stdlib.h>

// Entity systems need to be heap allocated since they're fairly large
static EntitySystem *allocate_entity_system(void)
{
    EntitySystem *entity_sys = calloc(1, sizeof(EntitySystem));
    es_initialize(entity_sys);

    return entity_sys;
}

static void free_entity_system(EntitySystem *es)
{
    free(es);
}

TEST_CASE(es_basics)
{
    EntitySystem *entity_sys = allocate_entity_system();
    EntityWithID entity_with_id = es_create_entity(entity_sys, FACTION_NEUTRAL);

    REQUIRE(es_entity_exists(entity_sys, entity_with_id.id));

    Entity *entity = es_get_entity(entity_sys, entity_with_id.id);
    REQUIRE(entity == entity_with_id.entity);
    REQUIRE(es_try_get_entity(entity_sys, entity_with_id.id) == entity);

    REQUIRE(es_has_no_components(entity));

    REQUIRE(!es_entity_is_inactive(entity));
    REQUIRE(entity_id_equal(es_get_id_of_entity(entity_sys, entity), entity_with_id.id));

    free_entity_system(entity_sys);
}

TEST_CASE(es_schedule_for_removal)
{
    EntitySystem *entity_sys = allocate_entity_system();
    EntityWithID entity_with_id = es_create_entity(entity_sys, FACTION_NEUTRAL);

    es_schedule_entity_for_removal(entity_with_id.entity);

    REQUIRE(es_entity_is_inactive(entity_with_id.entity));

    free_entity_system(entity_sys);
}

TEST_CASE(es_adding_components_single)
{
    EntitySystem *entity_sys = allocate_entity_system();

    EntityWithID entity_with_id = es_create_entity(entity_sys, FACTION_NEUTRAL);
    Entity *entity = entity_with_id.entity;

    REQUIRE(!es_has_component(entity, LifetimeComponent));

    es_add_component(entity, LifetimeComponent);

    REQUIRE(es_has_component(entity, LifetimeComponent));

    free_entity_system(entity_sys);
}

TEST_CASE(es_adding_components_multiple)
{
    EntitySystem *entity_sys = allocate_entity_system();

    EntityWithID entity_with_id = es_create_entity(entity_sys, FACTION_NEUTRAL);
    Entity *entity = entity_with_id.entity;

    REQUIRE(!es_has_component(entity, LifetimeComponent));
    REQUIRE(!es_has_component(entity, SpriteComponent));
    REQUIRE(!es_has_component(entity, LightEmitter));

    es_add_component(entity, LifetimeComponent);
    es_add_component(entity, SpriteComponent);
    es_add_component(entity, LightEmitter);

    REQUIRE(es_has_component(entity, LifetimeComponent));
    REQUIRE(es_has_component(entity, SpriteComponent));
    REQUIRE(es_has_component(entity, LightEmitter));

    free_entity_system(entity_sys);
}

TEST_CASE(es_removing_components_single)
{
    EntitySystem *entity_sys = allocate_entity_system();

    EntityWithID entity_with_id = es_create_entity(entity_sys, FACTION_NEUTRAL);
    Entity *entity = entity_with_id.entity;

    es_add_component(entity, LifetimeComponent);
    es_remove_component(entity, LifetimeComponent);

    REQUIRE(!es_has_component(entity, LifetimeComponent));

    free_entity_system(entity_sys);
}

TEST_CASE(es_removing_components_multiple)
{
    EntitySystem *entity_sys = allocate_entity_system();

    EntityWithID entity_with_id = es_create_entity(entity_sys, FACTION_NEUTRAL);
    Entity *entity = entity_with_id.entity;

    es_add_component(entity, LifetimeComponent);
    es_add_component(entity, SpriteComponent);
    es_add_component(entity, LightEmitter);

    es_remove_component(entity, LifetimeComponent);
    es_remove_component(entity, SpriteComponent);
    es_remove_component(entity, LightEmitter);

    REQUIRE(!es_has_component(entity, LifetimeComponent));
    REQUIRE(!es_has_component(entity, SpriteComponent));
    REQUIRE(!es_has_component(entity, LightEmitter));

    free_entity_system(entity_sys);
}


TEST_CASE(es_components_zeroed)
{
    EntitySystem *entity_sys = allocate_entity_system();

    EntityWithID entity_with_id = es_create_entity(entity_sys, FACTION_NEUTRAL);
    Entity *entity = entity_with_id.entity;

    LifetimeComponent *lt = es_add_component(entity, LifetimeComponent);
    lt->time_to_live = 123.0f;

    es_remove_component(entity, LifetimeComponent);

    LifetimeComponent *lt2 = es_add_component(entity, LifetimeComponent);
    REQUIRE(lt2->time_to_live == 0.0f);

    free_entity_system(entity_sys);
}

TEST_CASE(es_removing_entities)
{
    EntitySystem *entity_sys = allocate_entity_system();

    EntityWithID entity_with_id = es_create_entity(entity_sys, FACTION_NEUTRAL);

    es_remove_entity(entity_sys, entity_with_id.id);
    REQUIRE(!es_entity_exists(entity_sys, entity_with_id.id));
    REQUIRE(es_try_get_entity(entity_sys, entity_with_id.id) == 0);

    free_entity_system(entity_sys);
}

TEST_CASE(es_create_multiple)
{
    EntitySystem *entity_sys = allocate_entity_system();

    EntityWithID entity_with_id1 = es_create_entity(entity_sys, FACTION_NEUTRAL);
    EntityWithID entity_with_id2 = es_create_entity(entity_sys, FACTION_NEUTRAL);

    EntityID id1 = entity_with_id1.id;
    EntityID id2 = entity_with_id2.id;

    REQUIRE(es_entity_exists(entity_sys, id1));
    REQUIRE(es_entity_exists(entity_sys, id2));

    REQUIRE(!entity_id_equal(id1, id2));

    es_remove_entity(entity_sys, id1);

    REQUIRE(!es_entity_exists(entity_sys, id1));
    REQUIRE(es_entity_exists(entity_sys, id2));

    es_remove_entity(entity_sys, id2);
    REQUIRE(!es_entity_exists(entity_sys, id2));

    free_entity_system(entity_sys);
}

TEST_CASE(es_create_lots)
{
    EntitySystem *es = allocate_entity_system();

    EntityID ids[MAX_ENTITIES] = {0};

    for (ssize i = 0; i < MAX_ENTITIES; ++i) {
        ids[i] = es_create_entity(es, FACTION_NEUTRAL).id;
    }

    for (ssize i = 0; i < MAX_ENTITIES; ++i) {
        REQUIRE(es_entity_exists(es, ids[i]));

        Entity *entity = es_get_entity(es, ids[i]);
        REQUIRE(entity != 0);
        REQUIRE(!es_entity_is_inactive(entity));
    }

    free_entity_system(es);
}

TEST_CASE(es_create_lots_remove_in_order)
{
    EntitySystem *es = allocate_entity_system();

    for (ssize repeat = 0; repeat < 3; ++repeat) {
        EntityID ids[MAX_ENTITIES] = {0};

        for (ssize i = 0; i < MAX_ENTITIES; ++i) {
            ids[i] = es_create_entity(es, FACTION_NEUTRAL).id;
        }

        for (ssize i = 0; i < MAX_ENTITIES; ++i) {
            es_remove_entity(es, ids[i]);
        }

        for (ssize i = 0; i < MAX_ENTITIES; ++i) {
            REQUIRE(!es_entity_exists(es, ids[i]));

            Entity *entity = es_try_get_entity(es, ids[i]);
            REQUIRE(!entity);
        }
    }

    free_entity_system(es);
}

TEST_CASE(es_create_lots_remove_reverse_order)
{
    EntitySystem *es = allocate_entity_system();

    for (ssize repeat = 0; repeat < 3; ++repeat) {
        EntityID ids[MAX_ENTITIES] = {0};

        for (ssize i = 0; i < MAX_ENTITIES; ++i) {
            ids[i] = es_create_entity(es, FACTION_NEUTRAL).id;
        }

        for (ssize i = MAX_ENTITIES - 1; i >= 0; --i) {
            es_remove_entity(es, ids[i]);
        }

        for (ssize i = 0; i < MAX_ENTITIES; ++i) {
            REQUIRE(!es_entity_exists(es, ids[i]));

            Entity *entity = es_try_get_entity(es, ids[i]);
            REQUIRE(!entity);
        }
    }

    free_entity_system(es);
}

TEST_CASE(es_create_lots_remove_unordered)
{
    EntitySystem *es = allocate_entity_system();
    ssize batch_size = MAX_ENTITIES / 4;

    for (ssize repeat = 0; repeat < 4; ++repeat) {
        EntityID ids[MAX_ENTITIES] = {0};

        for (ssize i = 0; i < MAX_ENTITIES; ++i) {
            ids[i] = es_create_entity(es, FACTION_NEUTRAL).id;
        }

        for (ssize i = 0; i < batch_size; ++i) {
            es_remove_entity(es, ids[batch_size * 0 + i]);
        }

        for (ssize i = batch_size - 1; i >= 0; --i) {
            es_remove_entity(es, ids[batch_size * 1 + i]);
        }

        for (ssize i = 0; i < batch_size; ++i) {
            es_remove_entity(es, ids[batch_size * 2 + i]);
        }

        for (ssize i = batch_size - 1; i >= 0; --i) {
            es_remove_entity(es, ids[batch_size * 3 + i]);
        }

        for (ssize i = 0; i < MAX_ENTITIES; ++i) {
            REQUIRE(!es_entity_exists(es, ids[i]));

            Entity *entity = es_try_get_entity(es, ids[i]);
            REQUIRE(!entity);
        }
    }

    free_entity_system(es);
}

TEST_CASE(es_clone_entity_same_es)
{
    EntitySystem *es = allocate_entity_system();

    EntityWithID first = es_create_entity(es, FACTION_NEUTRAL);

    es_add_component(first.entity, LifetimeComponent);
    es_add_component(first.entity, SpriteComponent);
    es_add_component(first.entity, LightEmitter);

    EntityWithID clone = es_clone_entity(es, first.entity);

    REQUIRE(clone.entity != first.entity);
    REQUIRE(!entity_id_equal(clone.id, first.id));

    REQUIRE(v2_eq(clone.entity->position, first.entity->position));
    REQUIRE(clone.entity->faction == first.entity->faction);

    REQUIRE(es_has_component(first.entity, LifetimeComponent));
    REQUIRE(es_has_component(first.entity, SpriteComponent));
    REQUIRE(es_has_component(first.entity, LightEmitter));

    REQUIRE(es_has_component(clone.entity, LifetimeComponent));
    REQUIRE(es_has_component(clone.entity, SpriteComponent));
    REQUIRE(es_has_component(clone.entity, LightEmitter));

    free_entity_system(es);
}

TEST_CASE(es_clone_entity_different_es)
{
    EntitySystem *es1 = allocate_entity_system();
    EntitySystem *es2 = allocate_entity_system();

    EntityWithID first = es_create_entity(es1, FACTION_NEUTRAL);

    es_add_component(first.entity, LifetimeComponent);
    es_add_component(first.entity, SpriteComponent);
    es_add_component(first.entity, LightEmitter);

    EntityWithID clone = es_clone_entity(es2, first.entity);

    REQUIRE(clone.entity != first.entity);

    // Entity IDs should now be equal since they were the first entities
    // in their respective entity systems
    REQUIRE(entity_id_equal(clone.id, first.id));

    REQUIRE(v2_eq(clone.entity->position, first.entity->position));
    REQUIRE(clone.entity->faction == first.entity->faction);

    REQUIRE(es_has_component(first.entity, LifetimeComponent));
    REQUIRE(es_has_component(first.entity, SpriteComponent));
    REQUIRE(es_has_component(first.entity, LightEmitter));

    REQUIRE(es_has_component(clone.entity, LifetimeComponent));
    REQUIRE(es_has_component(clone.entity, SpriteComponent));
    REQUIRE(es_has_component(clone.entity, LightEmitter));

    free_entity_system(es1);
    free_entity_system(es2);
}

TEST_CASE(es_move_entity_into_different_es)
{
    EntitySystem *es1 = allocate_entity_system();
    EntitySystem *es2 = allocate_entity_system();

    EntityWithID first = es_create_entity(es1, FACTION_NEUTRAL);

    es_add_component(first.entity, LifetimeComponent);
    es_add_component(first.entity, SpriteComponent);
    es_add_component(first.entity, LightEmitter);

    EntityWithID clone = es_copy_entity_into_other_entity_system(es2, first.entity);

    REQUIRE(clone.entity != first.entity);

    REQUIRE(entity_id_equal(clone.id, first.id));

    REQUIRE(v2_eq(clone.entity->position, first.entity->position));
    REQUIRE(clone.entity->faction == first.entity->faction);

    REQUIRE(es_has_component(first.entity, LifetimeComponent));
    REQUIRE(es_has_component(first.entity, SpriteComponent));
    REQUIRE(es_has_component(first.entity, LightEmitter));

    REQUIRE(es_has_component(clone.entity, LifetimeComponent));
    REQUIRE(es_has_component(clone.entity, SpriteComponent));
    REQUIRE(es_has_component(clone.entity, LightEmitter));

    free_entity_system(es1);
    free_entity_system(es2);
}

TEST_CASE(es_move_entity_into_different_es_many)
{
    EntitySystem *es1 = allocate_entity_system();
    EntitySystem *es2 = allocate_entity_system();

    EntityID ids_in_first_es[MAX_ENTITIES] = {0};

    for (ssize i = 0; i < MAX_ENTITIES; ++i) {
        EntityWithID e = es_create_entity(es1, FACTION_NEUTRAL);

        es_add_component(e.entity, LifetimeComponent);
        es_add_component(e.entity, SpriteComponent);
        es_add_component(e.entity, LightEmitter);

        ids_in_first_es[i] = e.id;
    }

    EntityID ids_in_second_es[MAX_ENTITIES] = {0};

    for (ssize i = 0; i < MAX_ENTITIES; ++i) {
        Entity *entity = es_get_entity(es1, ids_in_first_es[i]);
        EntityWithID clone = es_copy_entity_into_other_entity_system(es2, entity);

        ids_in_second_es[i] = clone.id;
    }

    for (ssize i = 0; i < MAX_ENTITIES; ++i) {
        Entity *entity1 = es_get_entity(es1, ids_in_first_es[i]);
        Entity *entity2 = es_get_entity(es2, ids_in_second_es[i]);

        REQUIRE(entity1 != entity2);

        REQUIRE(entity_id_equal(entity2->id, ids_in_second_es[i]));
        REQUIRE(entity_id_equal(entity1->id, entity2->id));

        REQUIRE(v2_eq(entity2->position, entity1->position));
        REQUIRE(entity2->faction == entity1->faction);

        REQUIRE(es_has_component(entity1, LifetimeComponent));
        REQUIRE(es_has_component(entity1, SpriteComponent));
        REQUIRE(es_has_component(entity1, LightEmitter));

        REQUIRE(es_has_component(entity2, LifetimeComponent));
        REQUIRE(es_has_component(entity2, SpriteComponent));
        REQUIRE(es_has_component(entity2, LightEmitter));
    }

    /* EntityWithID clone = es_copy_entity_into_other_entity_system(es2, first.entity); */

    /* REQUIRE(clone.entity != first.entity); */

    /* REQUIRE(entity_id_equal(clone.id, first.id)); */

    /* REQUIRE(v2_eq(clone.entity->position, first.entity->position)); */
    /* REQUIRE(clone.entity->faction == first.entity->faction); */

    /* REQUIRE(es_has_component(first.entity, LifetimeComponent)); */
    /* REQUIRE(es_has_component(first.entity, SpriteComponent)); */
    /* REQUIRE(es_has_component(first.entity, LightEmitter)); */

    /* REQUIRE(es_has_component(clone.entity, LifetimeComponent)); */
    /* REQUIRE(es_has_component(clone.entity, SpriteComponent)); */
    /* REQUIRE(es_has_component(clone.entity, LightEmitter)); */

    free_entity_system(es1);
    free_entity_system(es2);
}
