#ifndef ENTITY_SYSTEM_H
#define ENTITY_SYSTEM_H

#include "base/ring_buffer.h"
#include "entity.h"
#include "quad_tree.h"

#define MAX_ENTITIES 32


// TODO: Create try_get_component that can return 0, make get_component crash on 0

#define es_add_component(entity, type) ((type *)es_impl_add_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_remove_component(entity, type)      es_impl_remove_component(entity, ES_IMPL_COMP_ENUM_NAME(type))
#define es_get_component(entity, type) ((type *)es_impl_get_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))
#define es_has_component(entity, type)          es_has_components((entity), component_flag(type))
#define es_has_components(entity, flags)        (((entity)->active_components & (flags)) == (flags))
#define es_get_or_add_component(entity, type)  ((type *)es_impl_get_or_add_component(entity, ES_IMPL_COMP_ENUM_NAME(type)))

typedef struct {
    Entity             entity;
    EntityIndex        alive_entity_array_index;
    QuadTreeLocation   quad_tree_location;
    EntityGeneration   generation;
} EntitySlot;

DEFINE_STATIC_RING_BUFFER(EntityID, EntityIDQueue, MAX_ENTITIES);

typedef struct EntityStorage {
    EntitySlot     entity_slots[MAX_ENTITIES];
    EntityIDQueue  free_id_queue;
    EntityID       alive_entity_ids[MAX_ENTITIES];
    EntityIndex    alive_entity_count;
    QuadTree       quad_tree;
} EntitySystem;

EntityWithID es_spawn_entity(EntitySystem *es, EntityFaction faction);

void       es_initialize(EntitySystem *es, Rectangle world_area);
Entity    *es_get_entity(EntitySystem *es, EntityID id);
Entity    *es_try_get_entity(EntitySystem *es, EntityID id);
EntityID   es_get_id_of_entity(EntitySystem *es, Entity *entity);
b32        es_entity_exists(EntitySystem *es, EntityID entity_id);
void       es_schedule_entity_for_removal(Entity *entity);
void       es_remove_inactive_entities(EntitySystem *es, LinearArena *scratch);
EntityIDList es_get_entities_in_area(EntitySystem *es, Rectangle area, LinearArena *arena);
EntityIDList es_get_inactive_entities(EntitySystem *es, LinearArena *scratch);
Rectangle    es_get_entity_bounding_box(Entity *entity);

void      *es_impl_add_component(Entity *entity, ComponentType type);
void      *es_impl_get_component(Entity *entity, ComponentType type);
void      *es_impl_get_or_add_component(Entity *entity, ComponentType type);
void       es_impl_remove_component(Entity *entity, ComponentType type);

// NOTE: until this is called at end of every entity update, entity won't be colliding with others
// etc. This might cause issues if an entity is spawning other entities
void       es_update_entity_quad_tree_location(EntitySystem *es, Entity *entity, LinearArena *arena);

#endif //ENTITY_SYSTEM_H
