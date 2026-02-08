#ifndef ENTITY_H
#define ENTITY_H

#include "base/linear_arena.h"
#include "base/rgba.h"
#include "base/rectangle.h"
#include "components/component.h"
#include "entity/entity_arena.h"
#include "entity_faction.h"

typedef struct Entity {
    EntityID id;

    /* This arena should be used by any components that require dynamically
       sized allocations. The arena buffer is stored in place inside the entity
       and pointers into it are only indices. As long as only pointers of that
       kind are used, entities should be trivially serializable/copyable/movable.
     */
    EntityArena entity_arena;

    ComponentBitset active_components;
    b32 is_inactive;

    EntityFaction faction;
    EntityState state;

    #define COMPONENT(type) type ES_IMPL_COMP_FIELD_NAME(type);
        COMPONENT_LIST
    #undef COMPONENT
} Entity;

typedef struct {
    Entity *entity;
    EntityID id;
} EntityWithID;

#endif //ENTITY_H
