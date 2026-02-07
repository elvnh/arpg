#include "entity_system.h"
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

static EntityIDSlot *get_next_id_slot(EntitySystem *es, EntityIDSlot id_slot)
{
    EntityIDSlot *result = 0;

    if (id_slot.next_free_id_index >= 0) {
        result = &es->id_slots[id_slot.next_free_id_index];
    }

    return result;
}

// TODO: return null if failed
static EntityIDSlot *get_first_free_id_slot(EntitySystem *es)
{
    EntitySlotID index = es->first_free_id_index;

    ASSERT(index >= 0);
    ASSERT(index < MAX_ENTITIES);

    EntityIDSlot *result = &es->id_slots[index];

    return result;
}


static void remove_id_slot(EntitySystem *es, EntityIDSlot *id_slot)
{
    ASSERT(id_slot->prev_free_id_index == -1 && "Removal in middle of list not yet supported");
    es->first_free_id_index = id_slot->next_free_id_index;

    EntityIDSlot *next = get_next_id_slot(es, *id_slot);

    // NOTE: for now we only remove from front of list, so we only need to deal with
    // the prev link of the next id slot
    if (next) {
        next->prev_free_id_index = -1;
    }

    // NOTE: for debugging purposes
    id_slot->next_free_id_index = -1;
    id_slot->prev_free_id_index = -1;
}


static EntitySlot *es_get_entity_slot(Entity *entity)
{
    // NOTE: this is safe as long as entity is first member of EntitySlot
    ASSERT(offsetof(EntitySlot, entity) == 0);
    EntitySlot *result = (EntitySlot *)entity;

    return result;
}

static EntitySlot *es_get_entity_slot_by_id(EntitySystem *es, EntityID id)
{
    ASSERT(id.slot_id < MAX_ENTITIES);
    EntitySlot *result = &es->entity_slots[id.slot_id];

    return result;
}

static b32 entity_id_is_valid(EntitySystem *es, EntityID id)
{
    b32 result =
	(id.generation >= FIRST_ENTITY_GENERATION)
	&& (id.slot_id < MAX_ENTITIES)
        && (es->id_slots[id.slot_id].generation == id.generation);

    return result;
}

static EntityID get_entity_id_from_slot(EntitySystem *es, EntitySlot *slot)
{
    EntitySlotID index = (EntitySlotID)(slot - es->entity_slots);
    EntityGeneration generation = es->id_slots[index].generation;

    EntityID result = {
	.slot_id = index,
	.generation = generation
    };

    return result;
}

EntityID es_get_id_of_entity(EntitySystem *es, Entity *entity)
{
    ASSERT(entity);
    EntitySlot *slot = es_get_entity_slot(entity);
    EntityID id = get_entity_id_from_slot(es, slot);

    return id;
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

void es_initialize(EntitySystem *es)
{
    for (EntitySlotID i = 0; i < MAX_ENTITIES; ++i) {
        EntityIDSlot slot = {0};

        slot.generation = FIRST_ENTITY_GENERATION;
        slot.prev_free_id_index = i - 1;

        if (i == (MAX_ENTITIES - 1)) {
            slot.next_free_id_index = -1;
        } else {
            slot.next_free_id_index = i + 1;
        }

        es->id_slots[i] = slot;
    }
}

static EntityID get_new_entity_id(EntitySystem *es)
{
    s32 index = es->first_free_id_index;

    ASSERT((index != -1) && "Ran out of IDs");

    EntityIDSlot *id_slot = get_first_free_id_slot(es);
    remove_id_slot(es, id_slot);

    EntityID result = {0};
    result.slot_id = index;
    result.generation = id_slot->generation;

    return result;
}

// TODO: why is this and es_create_entity separate functions?
static inline EntityID create_entity(EntitySystem *es)
{
    EntityID id = get_new_entity_id(es);

    EntitySlot *slot = es_get_entity_slot_by_id(es, id);
    slot->entity = zero_struct(Entity);

    return id;
}

EntityWithID es_create_entity(EntitySystem *es, EntityFaction faction)
{
    EntityID id = create_entity(es);
    Entity *entity = es_get_entity(es, id);
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

    EntityIDSlot *id_slot = &es->id_slots[id.slot_id];
    ASSERT(id_slot->next_free_id_index == -1);
    ASSERT(id_slot->prev_free_id_index == -1);

    EntityGeneration new_generation;
    if (id.generation == LAST_ENTITY_GENERATION) {
        // TODO: what to do when generation overflows? inactivate slot?
        new_generation = FIRST_ENTITY_GENERATION;
    } else {
        new_generation = id.generation + 1;
    }

    id_slot->generation = new_generation;

    id_slot->next_free_id_index = es->first_free_id_index;
    es->first_free_id_index = id.slot_id;
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
        EntitySlot *slot = es_get_entity_slot_by_id(es, id);
        result = &slot->entity;
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
