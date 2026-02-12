#ifndef PARTICLE_SPAWNER_H
#define PARTICLE_SPAWNER_H

#include "particle.h"

typedef enum {
    PS_FLAG_INFINITE                = FLAG(0),
    PS_FLAG_EMITS_LIGHT             = FLAG(1),
    PS_FLAG_SPAWN_ALL_AT_ONCE       = FLAG(2),

    /* If not set, only the component will be removed */
    PS_FLAG_WHEN_DONE_REMOVE_ENTITY = FLAG(3),
} ParticleSpawnerFlag;

// TODO: particle velocity
// TODO: reduce size of this
typedef struct {
    ParticleSpawnerFlag flags;

    RGBA32 particle_color;
    f32 particle_size;
    f32 particle_lifetime;
    f32 particle_speed;
    f32 particles_per_second;
} ParticleSpawnerConfig;

typedef struct {
    ParticleSpawnerConfig config;
    s32 total_particle_count;
} ParticleSpawnerSetup;

typedef struct {
    ParticleSpawnerConfig config;
    s32 particles_left_to_spawn;
    f32 particle_timer;
} ParticleSpawner;

void update_particle_spawner(struct World *world, struct Entity *entity, ParticleSpawner *ps,
    struct PhysicsComponent *physics, f32 dt);
b32  particle_spawner_is_finished(ParticleSpawner *ps);
void initialize_particle_spawner(ParticleSpawner *ps, ParticleSpawnerConfig config, s32 particle_count);

#endif //PARTICLE_SPAWNER_H
