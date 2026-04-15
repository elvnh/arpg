#ifndef ENTITY_H
#define ENTITY_H

#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "components/component.h"
#include "entity/entity_arena.h"
#include "entity_faction.h"

typedef enum {
    ENTITY_KIND_PERSISTENT, // Stays when world is set to inactive
    ENTITY_KIND_TRANSIENT,  // Gets removed when world is set to inactive
} EntityKind;

typedef struct Entity {
    EntityID id;

    ComponentBitset active_components;
    b32 is_inactive;

    EntityFaction faction;
    EntityState state;
    EntityKind kind;

#define COMPONENT(type) type ES_IMPL_COMP_FIELD_NAME(type);
    COMPONENT_LIST
#undef COMPONENT
} Entity;

typedef struct {
    Entity *entity;
    EntityID id;
} EntityWithID;

#endif //ENTITY_H
