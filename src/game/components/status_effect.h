#ifndef STATUS_EFFECT_H
#define STATUS_EFFECT_H

#include "base/utils.h"
#include "damage.h"
#include "modifier.h"

typedef struct {
    Modifier modifier;
    f32 time_remaining;
} StatusEffect;

typedef struct {
    StatusEffect effects[64];
    s32 effect_count;
} StatusEffectComponent;

static inline StatusEffect *status_effects_add(StatusEffectComponent *c, Modifier modifier, f32 lifetime)
{
    ASSERT(c->effect_count < ARRAY_COUNT(c->effects));

    s32 index = c->effect_count++;

    StatusEffect *result = &c->effects[index];
    result->modifier = modifier;
    result->time_remaining = lifetime;

    return result;
}

static inline void status_effects_update(StatusEffectComponent *status_effects, f32 dt)
{
    for (s32 i = 0; i < status_effects->effect_count; ++i) {
        StatusEffect *effect = &status_effects->effects[i];

        if (effect->time_remaining <= 0.0f) {
            *effect = status_effects->effects[status_effects->effect_count - 1];
            status_effects->effect_count -= 1;

            // TODO: check to see that this doesn't happen in any other places,
	    // i.e swap-removing and then continuing the loop body even if count reached 0
            if (status_effects->effect_count == 0) {
                break;
            }
        }

        effect->time_remaining -= dt;
    }
}

#endif //STATUS_EFFECT_H
