#ifndef ENTITY_SYSTEM_H
#define ENTITY_SYSTEM_H

#include "base/ring_buffer.h"
#include "entity.h"

#define MAX_ENTITIES 32

// TODO: Create try_get_component that can return null, make get_component crash on null

#define es_add_component(entity, type) ((type *)es_impl_add_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_remove_component(entity, type)      es_impl_remove_component(entity, ES_IMPL_COMP_ENUM_NAME(type))
#define es_get_component(entity, type) ((type *)es_impl_get_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_has_component(entity, type)          es_has_components((entity), component_flag(type))
#define es_has_components(entity, flags)        (((entity)->active_components & (flags)) == (flags))
#define es_get_or_add_component(entity, type)  ((type *)es_impl_get_or_add_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))

// TODO: better naming wrt slots, ids, id slots etc.

typedef struct {
    EntityGeneration generation;

    s32 prev_free_id_index;
    s32 next_free_id_index;
} EntityIDSlot;

typedef struct {
    // TODO: this struct is no longer needed
    Entity             entity;
} EntitySlot;

DEFINE_STATIC_RING_BUFFER(EntityID, EntityIDQueue, MAX_ENTITIES);

typedef struct EntitySystem {
    EntitySlot     entity_slots[MAX_ENTITIES];
    EntityIDSlot   id_slots[MAX_ENTITIES];
    EntityIndex    first_free_id_index;
} EntitySystem;

void          es_initialize(EntitySystem *es);
EntityWithID  es_create_entity(EntitySystem *es, EntityFaction faction);
void	      es_remove_entity(EntitySystem *es, EntityID id);
b32           es_has_no_components(Entity *entity);
Entity       *es_get_entity(EntitySystem *es, EntityID id);
Entity       *es_try_get_entity(EntitySystem *es, EntityID id);
EntityID      es_get_id_of_entity(EntitySystem *es, Entity *entity);
b32           es_entity_exists(EntitySystem *es, EntityID entity_id);
void          es_schedule_entity_for_removal(Entity *entity);
b32	      es_entity_is_inactive(Entity *entity);

void         *es_impl_add_component(Entity *entity, ComponentType type);
void         *es_impl_get_component(Entity *entity, ComponentType type);
void         *es_impl_get_or_add_component(Entity *entity, ComponentType type);
void          es_impl_remove_component(Entity *entity, ComponentType type);

#endif //ENTITY_SYSTEM_H
