#include "entity_state.h"
#include "base/vector.h"
#include "world.h"

static b32 can_transition_to_state(Entity *entity, EntityState state)
{
    (void)state;
    b32 result = (entity->state.kind != ENTITY_STATE_ATTACKING);

    return result;
}

static void transition_out_of_current_state(World *world, Entity *entity)
{
    switch (entity->state.kind) {
	case ENTITY_STATE_ATTACKING: {
	    SpellID spell = entity->state.as.attacking.spell_being_cast;
	    Vector2 target_pos = entity->state.as.attacking.target_position;

	    magic_cast_spell(world, spell, entity, target_pos);
	} break;

	default: {
	} break;
    }
}

static void play_state_animation(World *world, Entity *entity, EntityState state)
{
    if (es_has_component(entity, AnimationComponent)) {
	AnimationComponent *anim_comp = es_get_component(entity, AnimationComponent);
	AnimationID next_anim = anim_comp->state_animations[state.kind];

	// Don't restart the animation if we're currently already playing it
	if (next_anim != anim_comp->current_animation.animation_id) {
	    // TODO: instead check if there is an animation for this state?
	    ASSERT(next_anim != ANIM_NULL);

	    if (state.kind == ENTITY_STATE_ATTACKING) {
		f32 cast_speed = (f32)state.as.attacking.cast_speed / 100.0f;
		f32 spell_cast_duration = get_spell_cast_duration(state.as.attacking.spell_being_cast);

		anim_comp->current_animation = anim_begin_animation_with_duration(next_anim,
		    spell_cast_duration, cast_speed);
	    } else {
		anim_comp->current_animation = anim_begin_animation(next_anim, 1.0f);
	    }

	    switch (state.kind) {
		case ENTITY_STATE_IDLE:
		case ENTITY_STATE_WALKING: {
		} break;

		case ENTITY_STATE_ATTACKING: {
		    entity->velocity = V2_ZERO;
		    entity->direction = v2_sub(state.as.attacking.target_position, entity->position);
		} break;

		    INVALID_DEFAULT_CASE;
	    }
	}
    }
}

void entity_transition_to_state(World *world, Entity *entity, EntityState state)
{
    if (state.kind == ENTITY_STATE_WALKING && v2_is_zero(state.as.walking.direction)) {
	state = state_idle();
    }

    transition_out_of_current_state(world, entity);

    entity->state = state;

    play_state_animation(world, entity, state);

    switch (state.kind) {
	case ENTITY_STATE_WALKING: {
	    ASSERT(!v2_is_zero(state.as.walking.direction));
	    entity->velocity = v2_mul_s(state.as.walking.direction, 220.0f);
	} break;

	default: {
	    entity->velocity = V2_ZERO;
	} break;
    }
}

b32 entity_try_transition_to_state(World *world, Entity *entity, EntityState state)
{
    b32 result = false;

    if (can_transition_to_state(entity, state)) {
	entity_transition_to_state(world, entity, state);
	result = true;
    }

    return result;
}
