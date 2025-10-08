#ifndef DAMAGE_H
#define DAMAGE_H

#include "health.h"

typedef struct {
    // TODO: elemental damage types
    // TODO: resistance penetration
    s64 fire_damage;
} Damage;

// TODO: move to separate file
typedef struct {
    DamageResistances base_resistances;
    DamageResistances flat_bonuses;
} ResistanceStats;

typedef enum {
    DMG_TYPE_FIRE,
} DamageType;

static inline s64 damage_sum(Damage damage)
{
    s64 result = 0;

    result += damage.fire_damage;

    return result;
}

static inline s64 extract_damage_of_type(Damage damage, DamageType type)
{
    switch (type) {
        case DMG_TYPE_FIRE: return damage.fire_damage;
    }

    ASSERT(0);
    return 0;
}

static inline s32 extract_resistance_of_type(DamageResistances resistances, DamageType type)
{
    switch (type) {
        case DMG_TYPE_FIRE: return resistances.fire_resistance;
    }

    ASSERT(0);
    return 0;
}

static inline s32 get_total_resistance_of_type(ResistanceStats resistances, DamageType type)
{
    s32 base_resistance = extract_resistance_of_type(resistances.base_resistances, type);
    s32 flat_bonus = extract_resistance_of_type(resistances.flat_bonuses, type);

    s32 result = base_resistance + flat_bonus;

    return result;
}

static inline s64 calculate_damage_after_resistance(Damage damage, ResistanceStats resistances, DamageType dmg_type)
{
    s64 damage_amount = extract_damage_of_type(damage, dmg_type);
    s32 total_resistance = get_total_resistance_of_type(resistances, dmg_type);
    // TODO: reduce total_resistance here by resistance penetration

    // TODO: round up or down?
    s64 result = (s64)((f64)damage_amount * (1.0 - (f64)total_resistance / 100.0));

    return result;
}

static inline s64 calculate_damage_received(ResistanceStats resistances, Damage damage)
{
    s64 result = 0;

    result += calculate_damage_after_resistance(damage, resistances, DMG_TYPE_FIRE);

    return result;
}

#endif //DAMAGE_H
