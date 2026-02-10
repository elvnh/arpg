#ifndef COMPONENT_H
#define COMPONENT_H

#include "base/rgba.h"
#include "base/utils.h"
#include "component_id.h"
#include "collision/collision_policy.h"
#include "damage.h"
#include "light.h"
#include "particle.h"
#include "platform/asset.h"
#include "collision/collider.h"
#include "stats.h"
#include "status_effect.h"
#include "equipment.h"
#include "inventory.h"
#include "animation.h"
#include "entity/entity_state.h"
#include "magic.h"
#include "event_listener.h"
#include "ai.h"
#include "health.h"

#define COMPONENT_LIST                          \
    COMPONENT(PhysicsComponent)                 \
    COMPONENT(ColliderComponent)                \
    COMPONENT(ParticleSpawner)                  \
    COMPONENT(SpriteComponent)                  \
    COMPONENT(LifetimeComponent)                \
    COMPONENT(StatsComponent)                   \
    COMPONENT(StatusEffectComponent)            \
    COMPONENT(ItemModifiers)			\
    COMPONENT(AnimationComponent)               \
    COMPONENT(SpellCasterComponent)		\
    COMPONENT(EventListenerComponent)		\
    COMPONENT(DamageFieldComponent)		\
    COMPONENT(AIComponent)			\
    COMPONENT(EffectApplierComponent)		\
    COMPONENT(HealthComponent)			\
    COMPONENT(LightEmitter)			\
    COMPONENT(InventoryStorable)                \
    COMPONENT(Inventory)			\
    COMPONENT(Equippable)                       \
    COMPONENT(Equipment)                     \

#define ES_IMPL_COMP_ENUM_NAME(type) COMP_##type
#define ES_IMPL_COMP_FIELD_NAME(type) component_##type

typedef enum {
    #define COMPONENT(type) ES_IMPL_COMP_ENUM_NAME(type),
        COMPONENT_LIST
        COMPONENT_COUNT,
    #undef COMPONENT
} ComponentType;

typedef struct PhysicsComponent {
    Vector2 position;
    Vector2 velocity;
    Vector2 direction;
} PhysicsComponent;

typedef struct {
    Sprite sprite;
} SpriteComponent;

typedef struct {
    f32 time_to_live;
} LifetimeComponent;

typedef struct {
    StatValues stats;
} StatsComponent;

typedef struct AnimationComponent {
    // TODO: don't store the animations per component
    AnimationID state_animations[ENTITY_STATE_COUNT];
    AnimationInstance current_animation;
} AnimationComponent;

typedef struct SpellCasterComponent {
    SpellID spellbook[SPELL_COUNT];
    ssize spell_count;
} SpellCasterComponent;

typedef struct {
    DamageInstance damage;
    RetriggerBehaviour retrigger_behaviour;
} DamageFieldComponent;

typedef struct {
    StatusEffectID effect;
    RetriggerBehaviour retrigger_behaviour;
} EffectApplierComponent;

typedef struct AIComponent {
    AIState current_state;
} AIComponent;

typedef struct {
    Health health;
} HealthComponent;

typedef struct {
    LightSource light;
} LightEmitter;

static inline String component_id_to_string(ComponentID id)
{
    switch (id) {
#define COMPONENT(name) case component_id(name): return str_lit(#name);
        COMPONENT_LIST
#undef COMPONENT
    }

    ASSERT(0);
    return null_string;
}

#endif //COMPONENT_H
