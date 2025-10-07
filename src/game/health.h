#ifndef HEALTH_H
#define HEALTH_H

#include "game/damage.h"

typedef struct {
    s64 hitpoints;
    s32 fire_resistance; // TODO: should resistances be here?
} Health;

static inline s64 calculate_damage_after_resistance(s64 damage_amount, s64 resistance)
{
    // TODO: round up or down?
    s64 result = (s64)((f32)damage_amount * (1.0f - (f32)resistance / 100.0f));

    return result;
}

static inline s64 calculate_damage_received(Health health, Damage damage)
{
    s64 result = 0;

    result += calculate_damage_after_resistance(damage.fire_damage, health.fire_resistance);

    return result;
}

#endif //HEALTH_H
