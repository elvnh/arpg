#include "ai.h"

#include "base/utils.h"
#include "components/component.h"
#include "entity/entity_faction.h"
#include "entity/entity_state.h"
#include "entity/entity_system.h"
#include "magic.h"
#include "stats.h"
#include "world/world.h"

#define AI_CHASE_DISTANCE  250.0f
#define AI_ATTACK_DISTANCE 200.0f

static AIState ai_state_idle(void)
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

static b32 is_valid_attack_target(Entity *self, Entity *other)
{
    b32 result = (other->faction != self->faction) && (other->faction != FACTION_NEUTRAL)
                 && es_has_component(other, StatsComponent);

    return result;
}

static void transition_to_ai_state(World *world, Entity *entity, AIComponent *ai,
    PhysicsComponent *physics, AIState new_state)
{
    ai->current_state = new_state;

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (new_state.kind) {
        case AI_STATE_IDLE: {
            entity_try_transition_to_state(world, entity, physics, state_idle());
        } break;

        case AI_STATE_CHASING: {
        } break;

            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;
}

static void update_ai_state_idle(
    World *world, Entity *entity, AIComponent *ai, PhysicsComponent *self_physics)
{
    // TODO: get entities in area around ourselves instead of checking everyone

    for (ssize i = 0; i < world->active_entity_count; ++i) {
        EntityID other_id = world->active_entity_ids[i];
        Entity *other = es_get_entity(&world->entity_system, other_id);
        PhysicsComponent *other_physics = es_try_get_component(other, PhysicsComponent);

        if (other_physics && is_valid_attack_target(entity, other)) {
            f32 distance = v2_dist(other_physics->position, self_physics->position);

            if (distance < AI_CHASE_DISTANCE) {
                transition_to_ai_state(
                    world, entity, ai, self_physics, ai_state_chasing(other_id));
                break;
            }
        }
    }
}

static void update_ai_state_chasing(
    World *world, Entity *entity, AIComponent *ai, PhysicsComponent *self_physics)
{
    ASSERT(ai->current_state.kind == AI_STATE_CHASING);

    Entity *target =
        es_try_get_entity(&world->entity_system, ai->current_state.as.chasing.target);
    PhysicsComponent *target_physics = es_try_get_component(target, PhysicsComponent);
    ASSERT(target != entity);

    if (!target || !target_physics) {
        transition_to_ai_state(world, entity, ai, self_physics, ai_state_idle());
    } else {
        f32 distance = v2_dist(self_physics->position, target_physics->position);

        if (distance < AI_ATTACK_DISTANCE) {
            SpellCasterComponent *caster = es_get_component(entity, SpellCasterComponent);
            ASSERT(caster->spell_count > 0);

            // TODO: don't always cast first spell
            SpellID spell = get_spell_at_spellbook_index(caster, 0);

            try_cast_spell(world, spell, entity, target_physics->position);
        } else if (distance < AI_CHASE_DISTANCE) {
            Vector2 direction = v2_sub(target_physics->position, self_physics->position);
            direction = v2_norm(direction);

            entity_try_transition_to_state(
                world, entity, self_physics, state_walking(direction));
        } else {
            transition_to_ai_state(world, entity, ai, self_physics, ai_state_idle());
        }
    }
}

static void ai_behaviour_normal_enemy(
    World *world, Entity *entity, AIComponent *ai, PhysicsComponent *physics)
{
    BEGIN_EXHAUSTIVE_SWITCH;
    switch (ai->current_state.kind) {
        case AI_STATE_IDLE: {
            update_ai_state_idle(world, entity, ai, physics);
        } break;

        case AI_STATE_CHASING: {
            update_ai_state_chasing(world, entity, ai, physics);
        } break;
    }
    END_EXHAUSTIVE_SWITCH;
}

void entity_update_ai(World *world, Entity *entity, AIComponent *ai)
{
    PhysicsComponent *physics = es_try_get_component(entity, PhysicsComponent);

    if (!physics) {
        // Can't really do much if non-spatial, at least for now
        return;
    }

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (ai->behaviour) {
        case AI_BEHAVIOUR_NORMAL_ENEMY: {
            ai_behaviour_normal_enemy(world, entity, ai, physics);
        } break;

            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;
}
