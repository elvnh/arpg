#include "entity_system.h"
#include "base/linear_arena.h"
#include "base/ring_buffer.h"
#include "base/sl_list.h"
#include "base/utils.h"
#include "game/entity.h"
#include "game/quad_tree.h"

#define ES_MEMORY_SIZE MB(2)

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

void es_initialize(EntitySystem *es, Rectangle world_area)
{
    id_queue_initialize(&es->free_id_queue);
    qt_initialize(&es->quad_tree, world_area);

    for (EntityIndex i = FIRST_VALID_ENTITY_INDEX; i <= LAST_VALID_ENTITY_INDEX; ++i) {
        EntityID new_id = {
            .slot_id = (s32)i,
            .generation = 0
        };

        id_queue_push(&es->free_id_queue, new_id);
    }
}


static EntityID get_new_entity_id(EntitySystem *es)
{
    EntityID result = id_queue_pop(&es->free_id_queue);
    ASSERT(entity_id_is_valid(es, result));

    return result;
}


static inline EntityID create_entity(EntitySystem *es)
{
    EntityID id = get_new_entity_id(es);

    EntityIndex alive_index = es->alive_entity_count++;
    es->alive_entity_ids[alive_index] = id;

    EntitySlot *slot = es_get_entity_slot_by_id(es, id);
    slot->entity = (Entity){0};
    slot->alive_entity_array_index = alive_index;
    slot->quad_tree_location = (QuadTreeLocation){0};

    ASSERT(slot->generation == id.generation);

    return id;
}

EntityWithID es_spawn_entity(EntitySystem *es, EntityFaction faction)
{
    EntityID id = create_entity(es);
    Entity *entity = es_get_entity(es, id);

    entity->faction = faction;

    EntityWithID result = {
        .entity = entity,
        .id = id
    };

    return result;
}

static void es_remove_entity_from_quad_tree(QuadTree *qt, EntitySystem *es, Entity *entity)
{
    EntitySlot *slot = es_get_entity_slot(entity);
    EntityID id = get_entity_id_from_slot(es, slot);

    qt_remove_entity(qt, id, slot->quad_tree_location);
    slot->quad_tree_location = QT_NULL_LOCATION;
}

void es_remove_entity(EntitySystem *es, EntityID id)
{
    // TODO: clean up this function
    ASSERT(entity_id_is_valid(es, id));

    EntitySlot *removed_slot = es_get_entity_slot_by_id(es, id);
    ASSERT(removed_slot->alive_entity_array_index < es->alive_entity_count);

    EntityID *removed_id = &es->alive_entity_ids[removed_slot->alive_entity_array_index];
    EntityID *last_alive =  &es->alive_entity_ids[es->alive_entity_count - 1];
    ASSERT(removed_id->slot_id == id.slot_id);
    ASSERT(removed_id->generation == id.generation);
    EntitySlot *last_alive_slot = es_get_entity_slot_by_id(es, *last_alive);

    *removed_id = *last_alive;
    es->alive_entity_count--;

    last_alive_slot->alive_entity_array_index = removed_slot->alive_entity_array_index;

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

    es_remove_entity_from_quad_tree(&es->quad_tree, es, &removed_slot->entity);
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

void es_set_entity_area(EntitySystem *es, Entity *entity, Rectangle rectangle, LinearArena *arena)
{
    EntitySlot *slot = es_get_entity_slot(entity);
    EntityID id = get_entity_id_from_slot(es, slot);

    slot->quad_tree_location = qt_set_entity_area(&es->quad_tree, id,
	slot->quad_tree_location, rectangle, arena);
}

void es_set_entity_position(EntitySystem *es, Entity *entity, Vector2 new_pos, LinearArena *arena)
{
    EntitySlot *slot = es_get_entity_slot(entity);
    EntityID id = get_entity_id_from_slot(es, slot);

    slot->quad_tree_location = qt_move_entity(&es->quad_tree, id, slot->quad_tree_location,
	new_pos, arena);
}


b32 es_entity_exists(EntitySystem *es, EntityID entity_id)
{
    b32 result = es_get_entity(es, entity_id) != 0;

    return result;
}

EntityIDList es_get_inactive_entities(EntitySystem *es, LinearArena *scratch)
{
    EntityIDList result = {0};

    for (EntityIndex i = 0; i < es->alive_entity_count; ++i) {
        EntityID id = es->alive_entity_ids[i];
        Entity *entity = es_get_entity(es, id);

        if (entity->is_inactive) {
            EntityIDNode *node = la_allocate_item(scratch, EntityIDNode);
            node->id = id;

            sl_list_push_back(&result, node);
        }
    }

    return result;
}

void es_remove_inactive_entities(EntitySystem *es, LinearArena *scratch)
{
    EntityIDList to_remove = es_get_inactive_entities(es, scratch);

    for (EntityIDNode *node = list_head(&to_remove); node; node = list_next(node)) {
        es_remove_entity(es, node->id);
    }
}

void es_schedule_entity_for_removal(Entity *entity)
{
    entity->is_inactive = true;
}

EntityIDList es_get_entities_in_area(EntitySystem *es, Rectangle area, LinearArena *arena)
{
    EntityIDList result = qt_get_entities_in_area(&es->quad_tree, area, arena);
    return result;
}

Rectangle es_get_entity_bounding_box(Entity *entity)
{
    // NOTE: this minimum size is completely arbitrary
    Vector2 size = {4, 4};

    if (es_has_component(entity, ColliderComponent)) {
        ColliderComponent *collider = es_get_component(entity, ColliderComponent);
        size.x = MAX(size.x, collider->size.x);
        size.y = MAX(size.y, collider->size.y);
    }

    if (es_has_component(entity, SpriteComponent)) {
        SpriteComponent *sprite = es_get_component(entity, SpriteComponent);
        size.x = MAX(size.x, sprite->size.x);
        size.y = MAX(size.y, sprite->size.y);
    }

    Rectangle result = {entity->position, size};

    return result;
}

void es_update_entity_quad_tree_location(EntitySystem *es, Entity *entity, LinearArena *arena)
{
    EntitySlot *slot = es_get_entity_slot(entity);
    EntityID id = es_get_id_of_entity(es, entity);

    Rectangle new_area = es_get_entity_bounding_box(entity);
    slot->quad_tree_location = qt_set_entity_area(&es->quad_tree, id, slot->quad_tree_location, new_area, arena);
}
