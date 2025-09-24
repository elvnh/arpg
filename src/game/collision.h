#ifndef COLLISION_H
#define COLLISION_H

#include "base/vector.h"
#include "base/rectangle.h"

typedef struct {
    Vector2  new_position_a;
    Vector2  new_position_b;

    Vector2  new_velocity_a;
    Vector2  new_velocity_b;

    f32      movement_fraction_left;

    b32 are_colliding;
} CollisionInfo;

CollisionInfo collision_rect_vs_rect(f32 movement_fraction_left, Rectangle rect_a, Rectangle rect_b,
    Vector2 velocity_a, Vector2 velocity_b, f32 dt);

#endif //COLLISION_H
