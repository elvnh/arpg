#include "ai.h"
#include "components/component.h"
#include "entity_faction.h"
#include "entity_state.h"
#include "entity_system.h"
#include "magic.h"
#include "world.h"

static AIState ai_state_idle()
{
    AIState result = {0};
    result.kind = AI_STATE_IDLE;

    return result;
}

static AIState ai_state_chasing(EntityID target_id)
{
    AIState result = {0};
    result.kind = AI_STATE_CHASING;
    result.as.chasing.target = target_id;

    return result;
}


static void transition_to_ai_state(World *world, Entity *entity, AIComponent *ai, AIState new_state)
{
    ai->current_state = new_state;

    switch (new_state.kind) {
	case AI_STATE_IDLE: {
	    entity_try_transition_to_state(world, entity, state_idle());
	} break;

	case AI_STATE_CHASING: {
	} break;
    }
}

#define AI_CHASE_DISTANCE 250.0f
#define AI_ATTACK_DISTANCE 200.0f

static void update_ai_state_idle(World *world, Entity *entity, AIComponent *ai)
{
    // TODO: get entities in area around ourselves instead of checking everyone

    for (ssize i = 0; i < world->alive_entity_count; ++i) {
	EntityID other_id = world->alive_entity_ids[i];
	Entity *other = es_get_entity(world->entity_system, other_id);

	if ((other->faction != entity->faction) && (other->faction != FACTION_NEUTRAL)) {
	    f32 distance = v2_dist(other->position, entity->position);

	    if (distance < AI_CHASE_DISTANCE) {
		transition_to_ai_state(world, entity, ai, ai_state_chasing(other_id));
		break;
	    }
	}
    }
}

static void update_ai_state_chasing(World *world, Entity *entity, AIComponent *ai)
{
    ASSERT(ai->current_state.kind == AI_STATE_CHASING);

    Entity *target = es_try_get_entity(world->entity_system, ai->current_state.as.chasing.target);
    ASSERT(target != entity);

    if (!target) {
	transition_to_ai_state(world, entity, ai, ai_state_idle());
    } else {
	f32 distance = v2_dist(entity->position, target->position);

	if (distance < AI_ATTACK_DISTANCE) {
	    SpellCasterComponent *caster = es_get_component(entity, SpellCasterComponent);
	    ASSERT(caster);
	    ASSERT(caster->spell_count > 0);

	    SpellID spell = get_spell_at_spellbook_index(caster, 0);
	    // TODO: get actual stat value
	    StatValue cast_speed = 100;

	    entity_try_transition_to_state(world, entity, state_attacking(spell,
		    target->position, cast_speed));
	} else if (distance < AI_CHASE_DISTANCE) {
	    Vector2 direction = v2_sub(target->position, entity->position);
	    direction = v2_norm(direction);

	    entity_try_transition_to_state(world, entity, state_walking(direction));
	} else {
	    transition_to_ai_state(world, entity, ai, ai_state_idle());
	}
    }
}

void entity_update_ai(World *world, Entity *entity, AIComponent *ai)
{
    switch (ai->current_state.kind) {
	case AI_STATE_IDLE: {
	    update_ai_state_idle(world, entity, ai);
	} break;

	case AI_STATE_CHASING: {
	    update_ai_state_chasing(world, entity, ai);
	} break;
    }
}
