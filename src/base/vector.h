#ifndef VECTOR2_H
#define VECTOR2_H

#include "typedefs.h"

typedef struct {
    f32 x;
    f32 y;
} Vector2;

typedef struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} Vector4;

static inline Vector2 v2_add(Vector2 lhs, Vector2 rhs)
{
    Vector2 result = {
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y
    };

    return result;
}

#endif //VECTOR2_H
