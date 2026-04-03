#include "procedural_animation.h"

#include "base/utils.h"
#include "components/component.h"
#include "entity/entity_system.h"
#include "world/world.h"

static Vector2 dispatch_position_animation(PositionAnimation func, Vector2 start,
    PositionAnimationArgs args, f32 t)
{
    Vector2 result = {0};

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (func) {
        case POSITION_ANIMATION_PARABOLA: {
            Vector2 a = args.parabola.middle;
            Vector2 b = v2_sub(v2_sub(args.parabola.end, start), a);

            result = v2_add(v2_add(v2_mul_s(a, t * t), v2_mul_s(b, t)), start);
        } break;

        case POSITION_ANIMATION_ORBIT: {
            f32 x = t * args.orbit.rotations_in_radians;

            result = v2_add(start, v2_mul_s(v2(cosf(x), sinf(x)), args.orbit.radius));
        } break;

            INVALID_CASE(POSITION_ANIMATION_NONE);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    return result;
}

static f32 dispatch_rotation_animation(RotationAnimation func, f32 start,
    RotationAnimationArgs args, f32 t)
{
    f32 result = 0.0f;

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (func) {
        case ROTATION_ANIMATION_LERP: {
            result = interpolate(start, args.lerp.rotations_in_radians, t);

        } break;

            INVALID_CASE(ROTATION_ANIMATION_NONE);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    return result;
}

void update_procedural_animation(World *world, ProceduralAnimation *anim,
    PhysicsComponent *physics, f32 dt)
{
    Entity *owning_entity =
        es_get_component_owner(&world->entity_system, physics, PhysicsComponent);

    ASSERT(anim->duration >= 0.0f);

    anim->elapsed_time += dt;
    anim->elapsed_time = CLAMP(anim->elapsed_time, 0.0f, anim->duration);

    f32 t = anim->elapsed_time / anim->duration;

    if (anim->position_function != POSITION_ANIMATION_NONE) {
        Vector2 start = anim->start_position;

        physics->visual_position_offset = dispatch_position_animation(anim->position_function,
            start, anim->position_args, t);
    }

    if (anim->rotation_animation != ROTATION_ANIMATION_NONE) {
        physics->visual_rotation = dispatch_rotation_animation(anim->rotation_animation,
            anim->start_rotation, anim->rotation_args, t);
    }

    if (anim->elapsed_time >= anim->duration) {
        BEGIN_EXHAUSTIVE_SWITCH;
        switch (anim->on_end_behaviour.kind) {
            case ANIM_ON_END_DO_NOTHING: {
            } break;

            case ANIM_ON_END_REPEAT: {
                anim->elapsed_time = 0;
            } break;

            case ANIM_ON_END_TRANSITION_TO_STATE: {
                entity_force_transition_to_state(world, owning_entity, physics,
                    anim->on_end_behaviour.as.state_transition);
            } break;

            case ANIM_ON_END_REMOVE_COMPONENT: {
                es_remove_component(owning_entity, AnimationComponent);
            } break;

                INVALID_DEFAULT_CASE;
        }
        END_EXHAUSTIVE_SWITCH;
    }
}
