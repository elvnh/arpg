#include "random.h"
#include "base/maths.h"

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
    rng_set_global_rng_state(rng);

    rng->state[0] = splitmix(&seed);
    rng->state[1] = splitmix(&seed);
    rng->state[2] = splitmix(&seed);
    rng->state[3] = splitmix(&seed);
}

void rng_set_global_rng_state(RNGState *rng)
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

s32 rng_s32(s32 min, s32 max)
{
    ASSERT(min <= max);
    u64 x = xoshiro256ss();
    f32 n = (f32)((f64)x / (f64)U64_MAX);

    s32 result = (min + (s32)((f32)(max - min) * (f32)n));

    ASSERT(result >= min);
    ASSERT(result <= max);

    return result;
}
