#ifndef PARTICLE_H
#define PARTICLE_H

#include "base/typedefs.h"

typedef struct Particle {
    f32 timer;
    f32 lifetime;
    Vector2 position;
    Vector2 velocity;
} Particle;

#endif //PARTICLE_H
