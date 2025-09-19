#ifndef ENTITY_H
#define ENTITY_H

#include "base/vector.h"
#include "base/rgba.h"

#define MAX_ENTITIES 32

/*
  TODO:
  - Component macros
  - Entity archetypes?
*/

typedef s32 EntityIndex;
typedef s32 EntityGeneration; 
typedef u64 ComponentBitset;

typedef enum {
    COMP_PhysicsComponent,
    COMP_ColliderComponent,
} ComponentType;

typedef struct {
    Vector2 position;
    Vector2 velocity;    
} PhysicsComponent;

typedef struct {
    Vector2 size;    
} ColliderComponent;

typedef struct {
    ComponentBitset   active_components;
    PhysicsComponent  physics_component;
    ColliderComponent collider_component;

    RGBA32 color;
} Entity;

typedef struct {
    EntityIndex alive_entity_array_index;
    EntityGeneration generation;
    Entity      entity;
} EntitySlot;

typedef struct {
    EntityIndex      slot_index;
    EntityGeneration generation;
} EntityID;

typedef struct {
    EntityID ids[MAX_ENTITIES];
    ssize head;
    ssize tail;
} EntityIDQueue;

typedef struct {
    EntitySlot     entities[MAX_ENTITIES];
    EntityIDQueue  free_id_queue;
    EntityID       alive_entity_ids[MAX_ENTITIES];
    EntityIndex    alive_entity_count; 
} EntityStorage;

void     es_initialize(EntityStorage *es);
EntityID es_create_entity(EntityStorage *es);
void     es_remove_entity(EntityStorage *es, EntityID id);
Entity  *es_get_entity(EntityStorage *es, EntityID id);
void    *es_add_component_impl(Entity *entity, ComponentType type); // TODO: allow providing initialized component
void    *es_get_component_impl(Entity *entity, ComponentType type);

#endif //ENTITY_H
