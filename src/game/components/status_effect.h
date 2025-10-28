#ifndef STATUS_EFFECT_H
#define STATUS_EFFECT_H

#include "base/utils.h"
#include "damage.h"

typedef enum {
    STATUS_EFFECT_DAMAGE_MODIFIER,
} StatusEffectKind;

typedef struct {
    StatusEffectKind kind;

    union {
        DamageValueModifiers damage_modifiers;
        //DamageValueModifiers resistance_modifiers;
    } as;
} StatusEffect;

typedef struct {
    StatusEffect effects[64];
    s32 effect_count;
} StatusEffectComponent;

static inline StatusEffect *status_effects_add(StatusEffectComponent *c, StatusEffectKind kind)
{
    ASSERT(c->effect_count < ARRAY_COUNT(c->effects));
    s32 index = c->effect_count++;
    StatusEffect *result = &c->effects[index];
    result->kind = kind;

    return result;
}

#endif //STATUS_EFFECT_H
