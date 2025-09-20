#ifndef ENTITY_H
#define ENTITY_H

#include "base/vector.h"
#include "base/rgba.h"
#include "component.h"

/*
  TODO:
  - Entity archetypes?
*/

#define MAX_ENTITIES 32

#define es_add_component(entity, type) ((type *)es_impl_add_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_get_component(entity, type) ((type *)es_impl_get_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_has_component(entity, type)          es_has_components((entity), component_flag(type))
#define es_has_components(entity, flags)        (((entity)->active_components & (flags)) == (flags))

typedef s32 EntityIndex;
typedef s32 EntityGeneration;

typedef struct {
    ComponentBitset active_components;
    RGBA32 color;

    #define COMPONENT(type) type ES_IMPL_COMP_FIELD_NAME(type);
        COMPONENT_LIST
    #undef COMPONENT
} Entity;

typedef struct {
    EntityIndex        alive_entity_array_index;
    EntityGeneration   generation;
    Entity             entity;
} EntitySlot;

typedef struct {
    EntityIndex      slot_index;
    EntityGeneration generation;
} EntityID;

typedef struct {
    EntityID   ids[MAX_ENTITIES];
    ssize      head;
    ssize      tail;
} EntityIDQueue;

typedef struct {
    EntitySlot     entities[MAX_ENTITIES];
    EntityIDQueue  free_id_queue;
    EntityID       alive_entity_ids[MAX_ENTITIES];
    EntityIndex    alive_entity_count;
} EntityStorage;

void       es_initialize(EntityStorage *es);
EntityID   es_create_entity(EntityStorage *es);
void       es_remove_entity(EntityStorage *es, EntityID id);
Entity    *es_get_entity(EntityStorage *es, EntityID id);
void      *es_impl_add_component(Entity *entity, ComponentType type); // TODO: allow providing initialized component?
void      *es_impl_get_component(Entity *entity, ComponentType type);

#endif //ENTITY_H
