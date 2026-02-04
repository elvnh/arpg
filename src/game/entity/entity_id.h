#ifndef ENTITY_ID_H
#define ENTITY_ID_H

#include "base/typedefs.h"

#define NULL_ENTITY_ID ((EntityID){0})

typedef s32 EntityIndex;
typedef s32 EntityGeneration;

// TODO: this shouldn't be a separate file

typedef struct {
    EntityIndex      slot_id;
    EntityGeneration generation;
} EntityID;


typedef struct {
    EntityID   entity_a;
    EntityID   entity_b;
} EntityPair;

typedef enum {
    ENTITY_PAIR_INDEX_FIRST,
    ENTITY_PAIR_INDEX_SECOND,
} EntityPairIndex;

static inline b32 entity_id_equal(EntityID lhs, EntityID rhs)
{
    b32 result = (lhs.slot_id == rhs.slot_id) && (lhs.generation == rhs.generation);

    return result;
}

static inline b32 entity_id_less_than(EntityID lhs, EntityID rhs)
{
    b32 result = lhs.slot_id < rhs.slot_id;

    return result;
}

static inline b32 entity_id_is_null(EntityID id)
{
    b32 result = entity_id_equal(id, NULL_ENTITY_ID);
    return result;
}

static inline u64 entity_id_hash(EntityID id)
{
    u64 result = (u64)id.slot_id ^ (u64)id.generation;

    return result;
}

static inline b32 entity_pair_equal(EntityPair lhs, EntityPair rhs)
{
    b32 result = entity_id_equal(lhs.entity_a, rhs.entity_a)
        && entity_id_equal(lhs.entity_b, rhs.entity_b);

    return result;
}

static inline EntityPair unordered_entity_pair(EntityID a, EntityID b)
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
    u64 result = (u32)pair.entity_a.slot_id ^ (u32)pair.entity_b.slot_id; // TODO: better hash

    return result;
}

#endif //ENTITY_ID_H
