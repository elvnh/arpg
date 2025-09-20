#include <string.h>

#include "entity.h"
#include "base/utils.h"
#include "game/component.h"

static ssize component_offsets[] = {
    #define COMPONENT(type) offsetof(Entity, ES_IMPL_COMP_FIELD_NAME(type)),
        COMPONENT_LIST
    #undef COMPONENT
};

static ssize get_component_offset(ComponentType type)
{
    ASSERT(type < COMPONENT_COUNT);
    ssize result = component_offsets[type];

    return result;
}

void *es_impl_add_component(Entity *entity, ComponentType type)
{
    ASSERT(!es_has_components(entity, ES_IMPL_COMP_ENUM_BIT_VALUE(type)));

    entity->active_components |= ES_IMPL_COMP_ENUM_BIT_VALUE(type);
    void *result = es_impl_get_component(entity, type);
    ASSERT(result);

    return result;
}

void *es_impl_get_component(Entity *entity, ComponentType type)
{
    if (!es_has_components(entity, type)) {
        return 0;
    }

    ssize component_offset = get_component_offset(type);
    void *result = byte_offset(entity, component_offset);

    return result;
}

static void id_queue_set_to_empty(EntityIDQueue *queue)
{
    queue->head = S_SIZE_MAX;
    queue->tail = S_SIZE_MAX;
}

static void id_queue_initialize(EntityIDQueue *queue)
{
    id_queue_set_to_empty(queue);
}

static b32 id_queue_is_empty(const EntityIDQueue *queue)
{
    b32 result = (queue->head == S_SIZE_MAX) && (queue->tail == S_SIZE_MAX);
    return result;
}

static b32 id_queue_is_full(const EntityIDQueue *queue)
{
    b32 result = (queue->head == queue->tail) && !id_queue_is_empty(queue);
    return result;
}

static void id_queue_push(EntityIDQueue *queue, EntityID id)
{
    ASSERT(!id_queue_is_full(queue));

    ssize push_index;

    if (id_queue_is_empty(queue)) {
        queue->head = 0;
        queue->tail = 1;
        push_index = 0;
    } else {
        push_index = queue->tail;
        queue->tail = (queue->tail + 1) % MAX_ENTITIES; // TODO: bitwise AND
    }

    queue->ids[push_index] = id;
}

static EntityID id_queue_pop(EntityIDQueue *queue)
{
    ASSERT(!id_queue_is_empty(queue));

    EntityID id = queue->ids[queue->head];
    queue->head = (queue->head + 1) % MAX_ENTITIES;

    if (queue->head == queue->tail) {
        id_queue_set_to_empty(queue);
    }

    return id;
}

void es_initialize(EntityStorage *es)
{
    id_queue_initialize(&es->free_id_queue);

    for (ssize i = 0; i < MAX_ENTITIES; ++i) {
        EntityID new_id = {
            .slot_index = (s32)i,
            .generation = 0
        };

        id_queue_push(&es->free_id_queue, new_id);
    }
}

static b32 entity_id_is_valid(EntityStorage *es, EntityID id)
{
    b32 result = (id.slot_index < MAX_ENTITIES)
        && (es->entities[id.slot_index].generation == id.generation);

    return result;
}

static EntityID get_new_entity_id(EntityStorage *es)
{
    EntityID result = id_queue_pop(&es->free_id_queue);
    return result;
}

EntitySlot *get_entity_slot(EntityStorage *es, EntityID id)
{
    ASSERT(id.slot_index < MAX_ENTITIES);
    EntitySlot *result = &es->entities[id.slot_index];

    return result;
}

EntityID es_create_entity(EntityStorage *es)
{
    EntityID id = get_new_entity_id(es);

    EntityIndex alive_index = es->alive_entity_count++;
    es->alive_entity_ids[alive_index] = id;

    EntitySlot *slot = get_entity_slot(es, id);
    slot->alive_entity_array_index = alive_index;
    slot->entity = (Entity){0};

    ASSERT(slot->generation == id.generation);

    return id;
}

void es_remove_entity(EntityStorage *es, EntityID id)
{
    ASSERT(entity_id_is_valid(es, id));

    EntitySlot *slot = get_entity_slot(es, id);
    ASSERT(slot->alive_entity_array_index < es->alive_entity_count);

    EntityID *removed_id = &es->alive_entity_ids[slot->alive_entity_array_index];
    EntityID *last_alive =  &es->alive_entity_ids[es->alive_entity_count - 1];
    ASSERT(removed_id->slot_index == id.slot_index);
    ASSERT(removed_id->generation == id.generation);
    *removed_id = *last_alive;
    es->alive_entity_count--;

    EntityGeneration new_generation;
    if (add_overflows_s32(id.generation, 1)) {
        // TODO: what to do when generation overflows? inactivate slot?
        new_generation = 0;
    } else {
        new_generation = id.generation + 1;
    }

    slot->generation = new_generation;
    id.generation = new_generation;

    id_queue_push(&es->free_id_queue, id);
}

Entity *es_get_entity(EntityStorage *es, EntityID id)
{
    ASSERT(entity_id_is_valid(es, id));
    Entity *result = &es->entities[id.slot_index].entity;

    return result;
}

Entity *es_get_entity_in_slot(EntityStorage *es, EntityIndex slot_index)
{
    EntityID id = {
        .slot_index = slot_index
    };

    Entity *result = es_get_entity(es, id);
    return result;
}
