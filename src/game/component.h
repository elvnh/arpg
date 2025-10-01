#ifndef COMPONENT_H
#define COMPONENT_H

#include "particle.h"
#include "asset.h"

#define COMPONENT_LIST                          \
    COMPONENT(PhysicsComponent)                 \
    COMPONENT(ColliderComponent)                \
    COMPONENT(DamageFieldComponent)             \
    COMPONENT(HealthComponent)                  \
    COMPONENT(ParticleSpawner)                  \

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
    Vector2 position;
    Vector2 velocity;
} PhysicsComponent;

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

typedef struct {
    Particle particles[1024];
    ssize particle_count;
    TextureHandle texture;
    RGBA32 particle_color;
    f32 particle_size;
} ParticleSpawner;

#endif //COMPONENT_H
