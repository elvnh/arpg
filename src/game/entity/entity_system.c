#include "entity_system.h"
#include "entity/entity_id.h"
#include "entity_arena.h"
#include "base/linear_arena.h"
#include "base/ring_buffer.h"
#include "base/sl_list.h"
#include "base/utils.h"
#include "components/component.h"
#include "world/quad_tree.h"

#define FIRST_ENTITY_GENERATION 1
#define LAST_ENTITY_GENERATION  S32_MAX

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

static ssize get_component_size(ComponentType type)
{
    switch (type) {
#define COMPONENT(type) case ES_IMPL_COMP_ENUM_NAME(type): return SIZEOF(type);
        COMPONENT_LIST
#undef COMPONENT

       INVALID_DEFAULT_CASE;
    }

    ASSERT(0);
    return 0;
}

static EntityIDSlot *get_id_slot_at_index(EntitySystem *es, EntityIndex index)
{
    ASSERT(index >= -1);

    EntityIDSlot *result = 0;

    if (index != -1) {
        result = &es->entity_ids[index];
    }

    return result;
}

static void remove_id_slot_from_free_list(EntitySystem *es, EntityIDSlot *id_slot)
{
    ASSERT(id_slot->prev_free_id_index == -1 && "Removal in middle of list not yet supported");
    es->first_free_id_index = id_slot->next_free_id_index;

    EntityIDSlot *next = get_id_slot_at_index(es, id_slot->next_free_id_index);

    // NOTE: for now we only remove from front of list, so we only need to deal with
    // the prev link of the next id slot
    if (next) {
        next->prev_free_id_index = -1;
    }

    // NOTE: for debugging purposes
    id_slot->next_free_id_index = -1;
    id_slot->prev_free_id_index = -1;
}

static b32 entity_id_is_valid(EntitySystem *es, EntityID id)
{
    b32 result =
	(id.generation >= FIRST_ENTITY_GENERATION)
	&& (id.index < MAX_ENTITIES)
        && (get_id_slot_at_index(es, id.index)->generation == id.generation);

    return result;
}

EntityID es_get_id_of_entity(EntitySystem *es, Entity *entity)
{
    ASSERT(entity);

    EntityIndex index = (EntityIndex)(entity - es->entities);
    EntityIDSlot *id_slot = get_id_slot_at_index(es, index);

    EntityID result = {0};
    result.index = index;
    result.generation = id_slot->generation;

    return result;
}

void es_initialize(EntitySystem *es)
{
    for (EntityIndex i = 0; i < MAX_ENTITIES; ++i) {
        EntityIDSlot slot = {0};

        slot.generation = FIRST_ENTITY_GENERATION;
        slot.prev_free_id_index = i - 1;

        if (i == (MAX_ENTITIES - 1)) {
            slot.next_free_id_index = -1;
        } else {
            slot.next_free_id_index = i + 1;
        }

        es->entity_ids[i] = slot;
    }
}

static EntityID get_new_entity_id(EntitySystem *es)
{
    EntityIDSlot *id_slot = get_id_slot_at_index(es, es->first_free_id_index);
    ASSERT(id_slot && "Ran out of IDs");

    EntityID result = {0};
    result.index = es->first_free_id_index;
    result.generation = id_slot->generation;

    remove_id_slot_from_free_list(es, id_slot);

    return result;
}

EntityWithID es_create_entity(EntitySystem *es, EntityFaction faction)
{
    EntityID id = get_new_entity_id(es);
    Entity *entity = es_get_entity(es, id);
    *entity = zero_struct(Entity);

    entity->faction = faction;

    ASSERT(entity_arena_get_memory_usage(&entity->entity_arena) == 0);

    EntityWithID result = {
        .entity = entity,
        .id = id
    };

    return result;
}

void es_remove_entity(EntitySystem *es, EntityID id)
{
    ASSERT(entity_id_is_valid(es, id));

    EntityIDSlot *id_slot = get_id_slot_at_index(es, id.index);
    ASSERT(id_slot->next_free_id_index == -1);
    ASSERT(id_slot->prev_free_id_index == -1);

    EntityGeneration new_generation = 0;
    if (id.generation == LAST_ENTITY_GENERATION) {
        // TODO: what to do when generation overflows? inactivate slot?
        new_generation = FIRST_ENTITY_GENERATION;
    } else {
        new_generation = id.generation + 1;
    }

    id_slot->generation = new_generation;

    id_slot->next_free_id_index = es->first_free_id_index;
    es->first_free_id_index = id.index;
}

Entity *es_get_entity(EntitySystem *es, EntityID id)
{
    Entity *result = es_try_get_entity(es, id);
    ASSERT(result);

    return result;
}

Entity *es_try_get_entity(EntitySystem *es, EntityID id)
{
    Entity *result = 0;

    if (entity_id_is_valid(es, id)) {
        result = &es->entities[id.index];
    }

    return result;
}

b32 es_entity_exists(EntitySystem *es, EntityID entity_id)
{
    b32 result = es_try_get_entity(es, entity_id) != 0;

    return result;
}

void es_schedule_entity_for_removal(Entity *entity)
{
    entity->is_inactive = true;
}

b32 es_entity_is_inactive(Entity *entity)
{
    b32 result = entity->is_inactive;

    return result;
}

b32 es_has_no_components(Entity *entity)
{
    b32 result = entity->active_components == 0;

    return result;
}

void *es_impl_add_component(Entity *entity, ComponentType type)
{
    ASSERT(!es_has_components(entity, ES_IMPL_COMP_ENUM_BIT_VALUE(type)));

    entity->active_components |= ES_IMPL_COMP_ENUM_BIT_VALUE(type);
    void *result = es_impl_get_component(entity, type);
    ASSERT(result);

    ssize size = get_component_size(type);
    memset(result, 0, (usize)size);

    return result;
}

void *es_impl_get_component(Entity *entity, ComponentType type)
{
    if (!es_has_components(entity, ES_IMPL_COMP_ENUM_BIT_VALUE(type))) {
        return 0;
    }

    ssize component_offset = get_component_offset(type);
    void *result = byte_offset(entity, component_offset);

    return result;
}

void *es_impl_get_or_add_component(Entity *entity, ComponentType type)
{
    void *component = es_impl_get_component(entity, type);

    if (!component) {
	component = es_impl_add_component(entity, type);
    }

    return component;
}

void es_impl_remove_component(Entity *entity, ComponentType type)
{
    u64 bit_value = ES_IMPL_COMP_ENUM_BIT_VALUE(type);
    ASSERT(es_has_components(entity, bit_value));

    entity->active_components &= ~(bit_value);
}
