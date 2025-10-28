#ifndef STATUS_EFFECT_H
#define STATUS_EFFECT_H

#include "base/utils.h"
#include "damage.h"

typedef enum {
    STATUS_EFFECT_DAMAGE_MODIFIER,
} StatusEffectKind;

typedef struct {
    StatusEffectKind kind;
    f32 expiry_time;

    union {
        DamageValueModifiers damage_modifiers;
        //DamageValueModifiers resistance_modifiers;
    } as;
} StatusEffect;

typedef struct {
    StatusEffect effects[64];
    s32 effect_count;
} StatusEffectComponent;

static inline StatusEffect *status_effects_add(StatusEffectComponent *c, StatusEffectKind kind, f32 lifetime)
{
    ASSERT(c->effect_count < ARRAY_COUNT(c->effects));
    s32 index = c->effect_count++;
    StatusEffect *result = &c->effects[index];
    result->kind = kind;
    result->expiry_time = lifetime;

    return result;
}

static inline void status_effects_update(StatusEffectComponent *status_effects, f32 dt)
{
    for (s32 i = 0; i < status_effects->effect_count; ++i) {
        StatusEffect *effect = &status_effects->effects[i];

        if (effect->expiry_time <= 0.0f) {
            *effect = status_effects->effects[status_effects->effect_count - 1];
            status_effects->effect_count -= 1;

            // TODO: check to see that this doesn't happen in any other places
            if (status_effects->effect_count == 0) {
                break;
            }
        }

        effect->expiry_time -= dt;
    }
}

#endif //STATUS_EFFECT_H
