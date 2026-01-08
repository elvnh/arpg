#include "random.h"
#include "base/maths.h"
#include "base/utils.h"


static RNGState *g_rng_state;

static u64 splitmix(u64 *state)
{
    *state += 0x9E3779B97F4A7C15;

    u64 result = *state;
    result = (result ^ (result >> 30)) * (u64)0xBF58476D1CE4E5B9;
    result = (result ^ (result >> 27)) * (u64)0x94D049BB133111EB;
    result = result ^ (result >> 31);

    return result;
}

void rng_initialize(RNGState *rng, u64 seed)
{
    rng_set_global_state(rng);

    rng->state[0] = splitmix(&seed);
    rng->state[1] = splitmix(&seed);
    rng->state[2] = splitmix(&seed);
    rng->state[3] = splitmix(&seed);
}

void rng_set_global_state(RNGState *rng)
{
    ASSERT(rng);
    g_rng_state = rng;
}

u64 xoshiro256ss()
{
    u64 *s = g_rng_state->state;

    u64 result = rotate_left_u64(s[1] * 5, 7) * 9;
    u64 x = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= x;
    s[3] = rotate_left_u64(s[3], 45);

    return result;
}

// TODO: mcros to make defining these simpler
s64 rng_s64(s64 min, s64 max)
{
    ASSERT(min <= max);
    u64 x = xoshiro256ss();
    f32 n = (f32)((f64)x / (f64)U64_MAX);

    s64 result = (s64)(min + (s64)((f32)(max - min) * n));

    ASSERT(result >= min);
    ASSERT(result <= max);

    return result;
}

s32 rng_s32(s32 min, s32 max)
{
    ASSERT(min <= max);
    u64 x = xoshiro256ss();
    f32 n = (f32)((f64)x / (f64)U64_MAX);

    s32 result = (min + (s32)((f32)(max - min) * n));

    ASSERT(result >= min);
    ASSERT(result <= max);

    return result;
}

f32 rng_f32(f32 min, f32 max)
{
    ASSERT(min <= max);
    u64 x = xoshiro256ss();
    f32 n = (f32)((f64)x / (f64)U64_MAX);

    f32 result = (min + ((f32)(max - min) * n));

    ASSERT(result >= min);
    ASSERT(result <= max);

    return result;
}

Vector2 rng_direction(f32 max_radians)
{
    f32 n = rng_f32(0, max_radians);
    f32 x = cos_f32(n);
    f32 y = sin_f32(n);

    Vector2 result = {x, y};
    return result;
}

Vector2 rng_position_in_rect(Rectangle rect)
{
    // TODO: this math isn't quite right

    f32 x_offs = rng_f32(-rect.size.x / 2.0f, rect.size.x / 2.0f);
    f32 y_offs = rng_f32(-rect.size.y / 2.0f, rect.size.y / 2.0f);

    Vector2 result = v2_add(
	v2_add(rect.position, v2_div_s(rect.size, 2.0f)),
	v2(x_offs, y_offs));

    return result;
}
