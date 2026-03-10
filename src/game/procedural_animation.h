#ifndef PROCEDURAL_ANIMATION_H
#define PROCEDURAL_ANIMATION_H

#include "animation.h"
#include "base/utils.h"
#include "base/vector.h"

struct PhysicsComponent;
struct World;

typedef enum {
    POSITION_ANIMATION_NONE,
    POSITION_ANIMATION_PARABOLA,
    POSITION_ANIMATION_ORBIT,
} PositionAnimation;

typedef enum {
    ROTATION_ANIMATION_NONE,
    ROTATION_ANIMATION_LERP,
} RotationAnimation;

typedef union {
    struct {
        Vector2 middle;
        Vector2 end;
    } parabola;

    struct {
        f32 radius;
        f32 rotations_in_radians;
    } orbit;
} PositionAnimationArgs;

typedef union {
    struct {
        f32 rotations_in_radians;
    } lerp;
} RotationAnimationArgs;

typedef struct {
    PositionAnimation position_function;
    Vector2 start_position;
    PositionAnimationArgs position_args;

    RotationAnimation rotation_animation;
    f32 start_rotation;
    RotationAnimationArgs rotation_args;

    f32 duration;
    f32 elapsed_time;
    AnimationOnEnd on_end_behaviour;
} ProceduralAnimation;

void update_procedural_animation(
    struct World *world, ProceduralAnimation *anim, struct PhysicsComponent *physics, f32 dt);

#endif //PROCEDURAL_ANIMATION_H
