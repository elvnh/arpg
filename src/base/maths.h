#ifndef MATHS_H
#define MATHS_H

#include <math.h>

#include "utils.h"

#define PI 3.1415926535f
#define PI_2 (PI / 2.0f)

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

// TODO: these are not needed, use sinf etc.
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

static inline f32 round_to_f32(f32 n, f32 multiple)
{
    ASSERT(multiple > 0.0f);

    f32 result = roundf(n / multiple) * multiple;

    return result;
}

static inline f32 interpolate(f32 a, f32 b, f32 t)
{
    // TODO: watch out for if result is too small to store in float, will result in NaN
    f32 result = (a * (1.0f - t)) + (b * t);

    return result;
}

static inline f32 interpolate_sin(f32 a, f32 b, f32 t)
{
    f32 result = interpolate(a, b, sin_f32(t * PI * 0.5f));

    return result;
}

static inline f32 exponential_moving_avg(f32 curr_avg, f32 new_data, f32 alpha)
{
    ASSERT(f32_in_range(alpha, 0.0f, 1.0f, 0.0f));
    f32 result = alpha * curr_avg + (1.0f - alpha) * new_data;

    return result;
}

static inline u64 rotate_left_u64(u64 n, ssize shift_count)
{
    ASSERT(shift_count < 64);
    u64 result = (n << shift_count) | (n >> (64 - shift_count));

    return result;
}

static inline f32 deg_to_rad(f32 degrees)
{
    f32 result = degrees * (PI / 180.0f);
    return result;
}

static inline f32 rad_to_deg(f32 radians)
{
    f32 result = radians * (180.0f / PI);
    return result;
}

static inline f32 fraction(f32 x)
{
    f32 result = (f32)(x - (f32)((s64)x));
    return fabsf(result);
}

#endif //MATHS_H
