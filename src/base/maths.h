#ifndef MATHS_H
#define MATHS_H

#include <math.h>

#include "typedefs.h"

static inline f32 abs_f32(f32 n)
{
    f32 result = (f32)fabs((f64)n);
    return result;
}

static inline f32 sqrt_f32(f32 n)
{
    f32 result = (f32)sqrt((f64)n);

    return result;
}

static inline f32 interpolate(f32 a, f32 b, f32 t)
{
    f32 result = (a * (1.0f - t)) + (b * t);

    return result;
}

#endif //MATHS_H
