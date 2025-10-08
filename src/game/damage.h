#ifndef DAMAGE_H
#define DAMAGE_H

#include "health.h"

typedef struct {
    // TODO: elemental damage types
    // TODO: resistance penetration
    s64 fire_damage;
} Damage;

static inline s64 damage_sum(Damage damage)
{
    s64 result = 0;

    result += damage.fire_damage;

    return result;
}

static inline s64 calculate_damage_after_resistance(s64 damage_amount, s64 resistance)
{
    // TODO: round up or down?
    s64 result = (s64)((f32)damage_amount * (1.0f - (f32)resistance / 100.0f));

    return result;
}

static inline s64 calculate_damage_received(DamageResistances resistances, Damage damage)
{
    s64 result = 0;

    result += calculate_damage_after_resistance(damage.fire_damage, resistances.fire_resistance);

    return result;
}


#endif //DAMAGE_H
