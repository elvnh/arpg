#ifndef ENTITY_H
#define ENTITY_H

#include "base/rgba.h"
#include "base/rectangle.h"
#include "components/component.h"

typedef s32 EntityIndex;
typedef s32 EntityGeneration;

typedef struct {
    EntityIndex      slot_id;
    EntityGeneration generation;
} EntityID;

typedef enum {
    FACTION_NEUTRAL, // TODO: implement neutral entities
    FACTION_PLAYER,
    FACTION_ENEMY,
} EntityFaction;

typedef struct Entity {
    ComponentBitset active_components;
    b32 is_inactive;

    EntityFaction faction;
    EntityState state;

    Vector2 position;
    Vector2 velocity;
    Vector2 direction;

    #define COMPONENT(type) type ES_IMPL_COMP_FIELD_NAME(type);
        COMPONENT_LIST
    #undef COMPONENT
} Entity;

typedef struct {
    Entity *entity;
    EntityID id;
} EntityWithID;

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

static inline b32 entity_pair_equal(EntityPair lhs, EntityPair rhs)
{
    b32 result = entity_id_equal(lhs.entity_a, rhs.entity_a)
        && entity_id_equal(lhs.entity_b, rhs.entity_b);

    return result;
}

static inline EntityPair entity_pair_create(EntityID a, EntityID b)
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

static inline u64 entity_id_hash(EntityID id)
{
    u64 result = (u64)id.slot_id ^ (u64)id.generation;

    return result;
}

// TODO: move elsewhere
static inline SpriteModifiers sprite_get_modifiers(Entity *entity, SpriteRotationBehaviour rotation_behaviour)
{
    f32 rotation = 0.0f;
    f32 dir_angle = (f32)atan2(entity->direction.y, entity->direction.x);

    RectangleFlip flip = RECT_FLIP_NONE;

    if (rotation_behaviour == SPRITE_ROTATE_BASED_ON_DIR) {
	rotation = dir_angle;
    } else if (rotation_behaviour == SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR) {
	b32 should_flip = (dir_angle > PI_2) || (dir_angle < -PI_2);

	if (should_flip) {
	    flip = RECT_FLIP_HORIZONTALLY;
	}
    }

    SpriteModifiers result = {
	.flip = flip,
	.rotation = rotation
    };

    return result;
}


#endif //ENTITY_H
