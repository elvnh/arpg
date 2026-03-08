#include "entity_state.h"

#include "base/vector.h"
#include "components/component.h"
#include "stats.h"
#include "world/world.h"

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

            spawn_spell_entities(world, spell, entity, target_pos);
        } break;

        default: {
        } break;
    }
}

static void play_state_animation(Entity *entity, EntityState state)
{
    FlipbookComponent *anim_comp = es_try_get_component(entity, FlipbookComponent);

    // TODO: make state transitions work without animation

    if (anim_comp) {
        PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);
        FlipbookID next_anim = anim_comp->state_animations[state.kind];

        // Don't restart the animation if we're currently already playing it
        if (next_anim != anim_comp->current_animation.animation_id) {
            // TODO: instead check if there is an animation for this state?
            ASSERT(next_anim != FLIPBOOK_NULL);

            if (state.kind == ENTITY_STATE_ATTACKING) {
                f32 cast_speed =
                    stat_value_percentage_as_factor(state.as.attacking.total_cast_speed);
                f32 spell_cast_duration =
                    get_spell_cast_duration(state.as.attacking.spell_being_cast);

                anim_comp->current_animation = begin_flipbook_animation_with_duration(
                    next_anim, spell_cast_duration, cast_speed);
            } else {
                anim_comp->current_animation = begin_flipbook_animation(next_anim, 1.0f);
            }

            BEGIN_EXHAUSTIVE_SWITCH;
            switch (state.kind) {
                case ENTITY_STATE_IDLE:
                case ENTITY_STATE_WALKING: {
                } break;

                case ENTITY_STATE_ATTACKING: {
                    physics->velocity = V2_ZERO;

                    physics->direction =
                        v2_sub(state.as.attacking.target_position, physics->position);
                } break;

                    INVALID_CASE(ENTITY_STATE_COUNT);
                    INVALID_DEFAULT_CASE;
            }
            END_EXHAUSTIVE_SWITCH;
        }
    }
}

void entity_force_transition_to_state(
    World *world, Entity *entity, PhysicsComponent *physics, EntityState state)
{
    if (state.kind == ENTITY_STATE_WALKING && v2_is_zero(state.as.walking.direction)) {
        state = state_idle();
    }

    // TODO: allow force transitioning without running "on exit"-effects
    transition_out_of_current_state(world, entity);

    entity->state = state;

    play_state_animation(entity, state);

    switch (state.kind) {
        case ENTITY_STATE_WALKING: {
            f32 speed = get_total_movement_speed_value(&world->entity_system, entity);

            physics->velocity = v2_mul_s(state.as.walking.direction, speed);
        } break;

        default: {
            physics->velocity = V2_ZERO;
        } break;
    }
}

b32 entity_try_transition_to_state(
    World *world, Entity *entity, PhysicsComponent *physics, EntityState state)
{
    b32 result = false;

    if (can_transition_to_state(entity, state)) {
        entity_force_transition_to_state(world, entity, physics, state);
        result = true;
    }

    return result;
}
