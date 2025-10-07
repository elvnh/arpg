#ifndef COMPONENT_H
#define COMPONENT_H

#include "health.h"
#include "particle.h"
#include "asset.h"

#define COMPONENT_LIST                          \
    COMPONENT(ColliderComponent)                \
    COMPONENT(DamageFieldComponent)             \
    COMPONENT(HealthComponent)                  \
    COMPONENT(ParticleSpawner)                  \
    COMPONENT(SpriteComponent)                  \
    COMPONENT(LifetimeComponent)                \
    COMPONENT(OnDeathComponent)                 \

#define ES_IMPL_COMP_ENUM_NAME(type) COMP_##type
#define ES_IMPL_COMP_FIELD_NAME(type) component_##type
#define ES_IMPL_COMP_ENUM_BIT_VALUE(e) ((u64)1 << (e))

#define component_flag(type) ES_IMPL_COMP_ENUM_BIT_VALUE(ES_IMPL_COMP_ENUM_NAME(type))

typedef u64 ComponentBitset;

typedef enum {
    #define COMPONENT(type) ES_IMPL_COMP_ENUM_NAME(type),
        COMPONENT_LIST
        COMPONENT_COUNT,
    #undef COMPONENT
} ComponentType;

typedef struct {
    Vector2 size;
    // TODO: offset from pos
    b32 non_blocking;
} ColliderComponent;

typedef struct {
    Damage damage;
} DamageFieldComponent;

typedef struct {
    Health health;
} HealthComponent;

// TODO: can be generalized
typedef enum {
    PS_WHEN_DONE_DO_NOTHING = 0,
    PS_WHEN_DONE_REMOVE_COMPONENT, // TODO: should this be default?
    PS_WHEN_DONE_REMOVE_ENTITY,
} ParticleSpawnerWhenDone;

// TODO: particle velocity
typedef struct {
    f32 particles_per_second;
    TextureHandle particle_texture;
    RGBA32 particle_color;
    f32 particle_size;
    f32 particle_lifetime;
    f32 particles_to_spawn; // TODO: this should be in ParticleSpawner
} ParticleSpawnerConfig;

// TODO: different kinds of spawners: spawn all at once, infinite particles etc,
// allow configuring angle of particles
typedef struct {
    ParticleBuffer particle_buffer;
    f32 particle_timer;
    f32 particles_left_to_spawn;
    ParticleSpawnerConfig config;
    ParticleSpawnerWhenDone action_when_done;
} ParticleSpawner;

static inline void particle_spawner_initialize(ParticleSpawner *ps, ParticleSpawnerConfig config)
{
    ring_initialize_static(&ps->particle_buffer);
    ps->config = config;
    ps->particles_left_to_spawn = config.particles_to_spawn;
}

typedef struct {
    TextureHandle texture;
    Vector2 size;
} SpriteComponent;

typedef struct {
    f32 time_to_live;
} LifetimeComponent;

typedef enum {
    DEATH_EFFECT_SPAWN_PARTICLES,
} OnDeathEffectKind;

typedef struct {
    ParticleSpawnerConfig config;
} DeathEffectSpawnParticles;

// TODO: multiple on death effects
typedef struct {
    OnDeathEffectKind kind;

    union {
        DeathEffectSpawnParticles spawn_particles;
    } as;
} OnDeathComponent;

#endif //COMPONENT_H
