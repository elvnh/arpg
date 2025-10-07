#ifndef COMPONENT_H
#define COMPONENT_H

#include "particle.h"
#include "asset.h"

#define COMPONENT_LIST                          \
    COMPONENT(ColliderComponent)                \
    COMPONENT(DamageFieldComponent)             \
    COMPONENT(HealthComponent)                  \
    COMPONENT(ParticleSpawner)                  \
    COMPONENT(SpriteComponent)                  \

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
    s32 damage;
} DamageFieldComponent;

typedef struct {
    s64 hitpoints;
} HealthComponent;

// TODO: can be generalized
typedef enum {
    PS_WHEN_DONE_DO_NOTHING = 0,
    PS_WHEN_DONE_REMOVE_COMPONENT, // TODO: should this be default?
    PS_WHEN_DONE_REMOVE_ENTITY,
} ParticleSpawnerWhenDone;

typedef struct {
    ParticleBuffer particle_buffer;
    f32 particle_timer;
    f32 particles_per_second;
    TextureHandle particle_texture;
    RGBA32 particle_color;
    f32 particle_size;
    f32 particle_lifetime;

    // TODO: allow infinite number of particles
    f32 particles_to_spawn;
    ParticleSpawnerWhenDone action_when_done;
} ParticleSpawner;

typedef struct {
    TextureHandle texture;
    Vector2 size;
} SpriteComponent;

#endif //COMPONENT_H
