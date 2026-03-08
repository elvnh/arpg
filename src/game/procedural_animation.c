#include "procedural_animation.h"

#include "base/utils.h"
#include "components/component.h"

static Vector2 dispatch_position_animation(
    PositionAnimation func, Vector2 start, PositionAnimationArgs args, f32 t)
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

static f32 dispatch_rotation_animation(
    RotationAnimation func, f32 start, RotationAnimationArgs args, f32 t)
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

void update_procedural_animation(
    ProceduralAnimation *anim, PhysicsComponent *physics, f32 dt)
{
    ASSERT(anim->duration >= 0.0f);

    anim->elapsed_time += dt;
    anim->elapsed_time = CLAMP(anim->elapsed_time, 0.0f, anim->duration);

    if (anim->is_looping && (anim->elapsed_time >= anim->duration)) {
        anim->elapsed_time = 0.0f;
    }

    f32 t = anim->elapsed_time / anim->duration;

    if (anim->position_function != POSITION_ANIMATION_NONE) {
        Vector2 start = anim->start_position;

        physics->visual_position_offset = dispatch_position_animation(
            anim->position_function, start, anim->position_args, t);
    }

    if (anim->rotation_animation != ROTATION_ANIMATION_NONE) {
        physics->visual_rotation = dispatch_rotation_animation(
            anim->rotation_animation, anim->start_rotation, anim->rotation_args, t);
    }
}
