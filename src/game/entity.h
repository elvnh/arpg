#ifndef ENTITY_H
#define ENTITY_H

#include "base/vector.h"
#include "base/rgba.h"
#include "component.h"
#include "entity_id.h"
#include "quad_tree.h"

#define MAX_ENTITIES 32

#define es_add_component(entity, type) ((type *)es_impl_add_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_get_component(entity, type) ((type *)es_impl_get_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_has_component(entity, type)          es_has_components((entity), component_flag(type))
#define es_has_components(entity, flags)        (((entity)->active_components & (flags)) == (flags))

typedef struct {
    ComponentBitset active_components;
    RGBA32 color;

    #define COMPONENT(type) type ES_IMPL_COMP_FIELD_NAME(type);
        COMPONENT_LIST
    #undef COMPONENT
} Entity;

typedef struct {
    Entity             entity;
    EntityIndex        alive_entity_array_index;
    QuadTreeLocation   quad_tree_location;
    EntityGeneration   generation;
} EntitySlot;

typedef struct {
    EntityID   ids[MAX_ENTITIES];
    ssize      head;
    ssize      tail;
} EntityIDQueue;

typedef struct EntityStorage {
    EntitySlot     entity_slots[MAX_ENTITIES];
    EntityIDQueue  free_id_queue;
    EntityID       alive_entity_ids[MAX_ENTITIES];
    EntityIndex    alive_entity_count;
    QuadTree       quad_tree;
} EntityStorage;

void       es_initialize(EntityStorage *es, Rectangle world_area);
EntityID   es_create_entity(EntityStorage *es);
void       es_remove_entity(EntityStorage *es, EntityID id);
Entity    *es_get_entity(EntityStorage *es, EntityID id);
void      *es_impl_add_component(Entity *entity, ComponentType type);
void      *es_impl_get_component(Entity *entity, ComponentType type);
void	   es_set_entity_area(EntityStorage *es, Entity *entity, Rectangle rectangle, LinearArena *arena);
void	   es_set_entity_position(EntityStorage *es, Entity *entity, Vector2 new_pos, LinearArena *arena);
void	   es_remove_entity_from_quad_tree(QuadTree *qt, EntityStorage *es, Entity *entity);
EntityID   es_get_id_of_entity(EntityStorage *es, Entity *entity);

#endif //ENTITY_H
