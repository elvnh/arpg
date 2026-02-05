#include "entity_system.h"
#include "base/linear_arena.h"
#include "base/ring_buffer.h"
#include "base/sl_list.h"
#include "base/utils.h"
#include "components/component.h"
#include "world/quad_tree.h"

#define ENTITY_ARENA_SIZE MB(1)

#define FIRST_VALID_ENTITY_INDEX (NULL_ENTITY_ID.slot_id + 1)
#define LAST_VALID_ENTITY_INDEX  (FIRST_VALID_ENTITY_INDEX + MAX_ENTITIES - 1)

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

static EntitySlot *es_get_entity_slot(Entity *entity)
{
    // NOTE: this is safe as long as entity is first member of EntitySlot
    ASSERT(offsetof(EntitySlot, entity) == 0);
    EntitySlot *result = (EntitySlot *)entity;

    return result;
}

static EntitySlot *es_get_entity_slot_by_id(EntitySystem *es, EntityID id)
{
    ASSERT(id.slot_id <= LAST_VALID_ENTITY_INDEX);
    EntitySlot *result = &es->entity_slots[id.slot_id - FIRST_VALID_ENTITY_INDEX];

    return result;
}

static b32 entity_id_is_valid(EntitySystem *es, EntityID id)
{
    b32 result =
	!entity_id_equal(id, NULL_ENTITY_ID)
	&& (id.slot_id <= LAST_VALID_ENTITY_INDEX)
        && (es_get_entity_slot_by_id(es, id)->generation == id.generation);

    return result;
}


static EntityID get_entity_id_from_slot(EntitySystem *es, EntitySlot *slot)
{
    EntityIndex index = (EntityIndex)(slot - es->entity_slots);
    EntityGeneration generation = slot->generation;

    EntityID result = {
	.slot_id = FIRST_VALID_ENTITY_INDEX + index,
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

    // TODO: zero out

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

static void id_queue_initialize(EntityIDQueue *queue)
{
    ring_initialize_static(queue);
}

static void id_queue_push(EntityIDQueue *queue, EntityID id)
{
    ring_push(queue, &id);
}

static EntityID id_queue_pop(EntityIDQueue *queue)
{
    EntityID result = ring_pop_load(queue);

    return result;
}

void es_initialize(EntitySystem *es, FreeListArena *world_arena)
{
    id_queue_initialize(&es->free_id_queue);

    for (EntityIndex i = FIRST_VALID_ENTITY_INDEX; i <= LAST_VALID_ENTITY_INDEX; ++i) {
        EntityID new_id = {
            .slot_id = (s32)i,
            .generation = 0
        };

        id_queue_push(&es->free_id_queue, new_id);
    }

    // Create all entity arenas up front, when creating an entity we'll just reset the
    // existing arena so it won't have to be created again
    for (ssize i = 0; i < MAX_ENTITIES; ++i) {
        EntitySlot *slot = &es->entity_slots[i];
        Entity *entity = &slot->entity;

        entity->entity_arena = la_create(fl_allocator(world_arena), ENTITY_ARENA_SIZE);
    }
}

static EntityID get_new_entity_id(EntitySystem *es)
{
    EntityID result = id_queue_pop(&es->free_id_queue);
    ASSERT(entity_id_is_valid(es, result));

    return result;
}

// TODO: why is this and es_create_entity separate functions?
static inline EntityID create_entity(EntitySystem *es)
{
    EntityID id = get_new_entity_id(es);

    EntitySlot *slot = es_get_entity_slot_by_id(es, id);

    // The arena is the only thing we wish to hold on to, zero out the rest
    LinearArena entity_arena = slot->entity.entity_arena;
    slot->entity = (Entity){0};
    slot->entity.entity_arena = entity_arena;

    ASSERT(slot->generation == id.generation);

    return id;
}

EntityWithID es_create_entity(EntitySystem *es, EntityFaction faction)
{
    EntityID id = create_entity(es);
    Entity *entity = es_get_entity(es, id);

    la_reset(&entity->entity_arena);
    entity->faction = faction;

    EntityWithID result = {
        .entity = entity,
        .id = id
    };

    return result;
}

void es_remove_entity(EntitySystem *es, EntityID id)
{
    ASSERT(entity_id_is_valid(es, id));

    EntitySlot *removed_slot = es_get_entity_slot_by_id(es, id);

    EntityGeneration new_generation;
    if (add_overflows_s32(id.generation, 1)) {
        // TODO: what to do when generation overflows? inactivate slot?
        new_generation = 0;
    } else {
        new_generation = id.generation + 1;
    }

    removed_slot->generation = new_generation;
    id.generation = new_generation;

    id_queue_push(&es->free_id_queue, id);
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
    b32 result = es_get_entity(es, entity_id) != 0;

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
