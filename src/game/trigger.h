#ifndef TRIGGER_H
#define TRIGGER_H

#include "entity_id.h"
#include "components/component_id.h"

// A trigger is any interaction between two entities. If this trigger needs a cooldown
// before it can be retriggered, a cooldown can be inserted into a table.
// For now, trigger cooldowns are per ordered entity pair, but it should be easy
// to encode additional information, so that for example cooldowns can be per component too.

#define should_invoke_trigger(world, entity, other, component)	\
    es_has_component(entity, component)				\
    && !trigger_is_on_cooldown(					\
	&world->trigger_cooldowns,				\
	es_get_id_of_entity(world->entity_system, entity),	\
	es_get_id_of_entity(world->entity_system, other),	\
	component_flag(component))

struct FreeListArena;
struct EntitySystem;
struct Entity;
struct World;

// TODO: retrigger after x amount of time

typedef enum {
    RETRIGGER_WHENEVER,
    RETRIGGER_NEVER,
    RETRIGGER_AFTER_NON_CONTACT,
} RetriggerBehaviour;

typedef struct TriggerCooldown {
    struct TriggerCooldown *next;
    struct TriggerCooldown *prev;

    EntityID owning_entity; // TODO: use ordered entity pair here
    EntityID collided_entity;
    ComponentBitset owning_component;

    RetriggerBehaviour retrigger_behaviour;
} TriggerCooldown;

typedef struct {
    TriggerCooldown *head;
    TriggerCooldown *tail;
} TriggerCooldownList;

typedef struct {
    TriggerCooldownList  table[512]; // TODO: dynamically sized
    TriggerCooldownList  free_node_list;
} TriggerCooldownTable;

void add_trigger_cooldown(TriggerCooldownTable *table, EntityID self, EntityID other,
    ComponentBitset component, RetriggerBehaviour retrigger_behaviour, struct FreeListArena *arena);
void remove_expired_trigger_cooldowns(struct World *world, struct EntitySystem *es);
b32 trigger_is_on_cooldown(TriggerCooldownTable *table, EntityID a, EntityID b, ComponentBitset component);

#endif //TRIGGER_H
