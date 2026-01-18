#ifndef ENTITY_STATE_H
#define ENTITY_STATE_H

#include "magic.h"
#include "stats.h"

struct World;
struct Entity;

typedef enum {
    ENTITY_STATE_IDLE,
    ENTITY_STATE_WALKING,
    ENTITY_STATE_ATTACKING,
    ENTITY_STATE_COUNT,
} EntityStateKind;

typedef struct {
    EntityStateKind kind;

    union {
	struct {
	    SpellID spell_being_cast;
	    Vector2 target_position;
	    StatValue cast_speed;
	} attacking;

	struct {
	    Vector2 direction;
	} walking;
    } as;
} EntityState;

void entity_transition_to_state(struct World *world, struct Entity *entity, EntityState state);
b32 entity_try_transition_to_state(struct World *world, struct Entity *entity, EntityState state);

static inline EntityState state_idle()
{
    EntityState result = {0};
    result.kind = ENTITY_STATE_IDLE;

    return result;
}

static inline EntityState state_walking(Vector2 direction)
{
    EntityState result = {0};
    result.kind = ENTITY_STATE_WALKING;
    result.as.walking.direction = direction;

    return result;
}

static inline EntityState state_attacking(SpellID spell_being_cast, Vector2 target_pos, StatValue cast_speed)
{
    EntityState result = {0};
    result.kind = ENTITY_STATE_ATTACKING;
    result.as.attacking.spell_being_cast = spell_being_cast;
    result.as.attacking.target_position = target_pos;
    result.as.attacking.cast_speed = cast_speed;

    return result;
}

#endif //ENTITY_STATE_H
