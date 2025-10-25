#ifndef RANDOM_H
#define RANDOM_H

#include "base/vector.h"

typedef struct {
    u64 state[4];
} RNGState;

void rng_initialize(RNGState *rng, u64 seed);
void rng_set_global_rng_state(RNGState *rng);
s32  rng_s32(s32 min, s32 max);
f32  rng_f32(f32 min, f32 max);
Vector2 rng_direction(f32 max_radians);

#endif //RANDOM_H
