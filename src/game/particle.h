#ifndef PARTICLE_H
#define PARTICLE_H

#include "base/ring_buffer.h"
#include "base/typedefs.h"
#include "base/vector.h"

typedef struct Particle {
    f32 timer;
    f32 lifetime;
    Vector2 position;
    Vector2 velocity;
} Particle;

DEFINE_STATIC_RING_BUFFER(Particle, ParticleBuffer, 1024);

#endif //PARTICLE_H
