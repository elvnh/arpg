#ifndef DAMAGE_H
#define DAMAGE_H

#include "health.h"

/*
  TODO:
  - Resistance penetration
  - Better way of setting resistance/damage values
  - Better naming
 */

typedef s64 DamageValue;

typedef enum {
    DMG_KIND_FIRE,
    DMG_KIND_COUNT,
} DamageKind;

typedef struct {
    DamageValue damage_types[DMG_KIND_COUNT]; // Use DamageKind as index
} DamageTypes;

typedef struct {
    DamageTypes base_resistances;
    DamageTypes flat_bonuses;
} ResistanceStats;

typedef struct {
    DamageTypes values;
} Damage;

static inline DamageValue get_damage_value_of_type(DamageTypes damages, DamageKind type)
{
    ssize index = (ssize)type;
    ASSERT((index >= 0) && (index < DMG_KIND_COUNT));

    DamageValue result = damages.damage_types[index];

    return result;
}

static inline DamageValue damage_sum(Damage damage)
{
    DamageValue result = 0;

    for (DamageKind kind = 0; kind < DMG_KIND_COUNT; ++kind) {
	result += get_damage_value_of_type(damage.values, kind);
    }

    return result;
}

static inline DamageValue get_total_resistance_of_type(ResistanceStats resistances, DamageKind type)
{
    DamageValue base_resistance = get_damage_value_of_type(resistances.base_resistances, type);
    DamageValue flat_bonus = get_damage_value_of_type(resistances.flat_bonuses, type);

    DamageValue result = base_resistance + flat_bonus;

    return result;
}

static inline DamageValue calculate_damage_of_type_after_resistance(Damage damage,
    ResistanceStats resistances, DamageKind dmg_type)
{
    DamageValue damage_amount = get_damage_value_of_type(damage.values, dmg_type);
    DamageValue total_resistance = get_total_resistance_of_type(resistances, dmg_type);

    // TODO: reduce total_resistance here by resistance penetration

    // TODO: round up or down?
    DamageValue result = (DamageValue)((f64)damage_amount * (1.0 - (f64)total_resistance / 100.0));

    return result;
}

static inline DamageValue calculate_damage_received(ResistanceStats resistances, Damage damage)
{
    DamageValue result = 0;

    for (DamageKind dmg_kind = 0; dmg_kind < DMG_KIND_COUNT; ++dmg_kind) {
	result += calculate_damage_of_type_after_resistance(damage, resistances, dmg_kind);
    }

    return result;
}

#endif //DAMAGE_H
