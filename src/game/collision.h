#ifndef COLLISION_H
#define COLLISION_H

#include "base/free_list_arena.h"
#include "base/vector.h"
#include "base/rectangle.h"
#include "base/linear_arena.h"

typedef enum {
    COLLISION_STATUS_NOT_COLLIDING,
    COLLISION_STATUS_ARE_INTERSECTING,
    COLLISION_STATUS_WILL_COLLIDE_THIS_FRAME,
} CollisionStatus;

typedef struct {
    CollisionStatus collision_status;
    Vector2         new_position_a;
    Vector2         new_position_b;
    Vector2         new_velocity_a;
    Vector2         new_velocity_b;
    f32             movement_fraction_left;
    Vector2         collision_normal; // TODO: separate normals for A and B?
} CollisionInfo;

CollisionInfo collision_rect_vs_rect(f32 movement_fraction_left, Rectangle rect_a, Rectangle rect_b,
    Vector2 velocity_a, Vector2 velocity_b, f32 dt);

#endif //COLLISION_H
