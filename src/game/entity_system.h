#ifndef ENTITY_SYSTEM_H
#define ENTITY_SYSTEM_H

#include "entity.h"
#include "quad_tree.h"

#define MAX_ENTITIES 32

#define es_add_component(entity, type) ((type *)es_impl_add_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_remove_component(entity, type)      es_impl_remove_component(entity, ES_IMPL_COMP_ENUM_NAME(type))
#define es_get_component(entity, type) ((type *)es_impl_get_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_has_component(entity, type)          es_has_components((entity), component_flag(type))
#define es_has_components(entity, flags)        (((entity)->active_components & (flags)) == (flags))

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
} EntitySystem;

void       es_initialize(EntitySystem *es, Rectangle world_area);
EntityID   es_create_entity(EntitySystem *es);
Entity    *es_get_entity(EntitySystem *es, EntityID id);
void      *es_impl_add_component(Entity *entity, ComponentType type);
void      *es_impl_get_component(Entity *entity, ComponentType type);
void       es_impl_remove_component(Entity *entity, ComponentType type);
void	   es_set_entity_area(EntitySystem *es, Entity *entity, Rectangle rectangle, LinearArena *arena);
void	   es_set_entity_position(EntitySystem *es, Entity *entity, Vector2 new_pos, LinearArena *arena);
EntityID   es_get_id_of_entity(EntitySystem *es, Entity *entity);
b32        es_entity_exists(EntitySystem *es, EntityID entity_id);
void       es_remove_inactive_entities(EntitySystem *es, LinearArena *scratch);
void       es_schedule_entity_for_removal(Entity *entity); // TODO: schedule_for_deactivation

#endif //ENTITY_SYSTEM_H
