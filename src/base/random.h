#ifndef RANDOM_H
#define RANDOM_H

#include "base/typedefs.h"

typedef struct {
    u64 state[4];
} RNGState;

void rng_initialize(RNGState *rng, u64 seed);
void rng_set_global_rng_state(RNGState *rng);
s32  rng_s32(s32 min, s32 max);

#endif //RANDOM_H
