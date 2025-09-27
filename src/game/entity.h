#ifndef ENTITY_H
#define ENTITY_H

#include "base/vector.h"
#include "base/rgba.h"
#include "component.h"

typedef s32 EntityIndex;
typedef s32 EntityGeneration;

typedef struct {
    EntityIndex      slot_index;
    EntityGeneration generation;
} EntityID;

typedef struct {
    ComponentBitset active_components;
    RGBA32 color;

    #define COMPONENT(type) type ES_IMPL_COMP_FIELD_NAME(type);
        COMPONENT_LIST
    #undef COMPONENT
} Entity;

typedef struct {
    EntityID   entity_a;
    EntityID   entity_b;
} EntityPair;

static inline b32 entity_id_equal(EntityID lhs, EntityID rhs)
{
    b32 result = (lhs.slot_index == rhs.slot_index) && (lhs.generation == rhs.generation);

    return result;
}

static inline b32 entity_id_less_than(EntityID lhs, EntityID rhs)
{
    b32 result = lhs.slot_index < rhs.slot_index;

    return result;
}

static inline EntityPair collision_pair_create(EntityID a, EntityID b)
{
    if (entity_id_less_than(b, a)) {
        EntityID tmp = a;
        a = b;
        b = tmp;
    }

    EntityPair result = {
	.entity_a = a,
	.entity_b = b
    };

    return result;
}

static inline u64 entity_pair_hash(EntityPair pair)
{
    u64 result = (u32)pair.entity_a.slot_index ^ (u32)pair.entity_b.slot_index; // TODO: better hash

    return result;
}

#endif //ENTITY_H
