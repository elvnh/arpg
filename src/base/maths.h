#ifndef MATHS_H
#define MATHS_H

#include <math.h>

#include "typedefs.h"

#define PI 3.1415926535f

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

static inline f32 sin_f32(f32 n)
{
    f32 result = (f32)sin((f64)n);
    return result;
}

static inline f32 cos_f32(f32 n)
{
    f32 result = (f32)cos((f64)n);

    return result;
}

static inline f32 interpolate(f32 a, f32 b, f32 t)
{
    // TODO: watch out for if result is too small to store in float,
    // will result in NaN
    f32 result = (a * (1.0f - t)) + (b * t);

    return result;
}

static inline f32 interpolate_sin(f32 a, f32 b, f32 t)
{
    f32 result = interpolate(a, b, sin_f32(t * PI * 0.5f));

    return result;
}

#endif //MATHS_H
