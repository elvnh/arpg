#ifndef DAMAGE_H
#define DAMAGE_H

#include "health.h"
#include "base/utils.h"
#include "base/random.h"

/*
  TODO:
  - Better way of setting resistance/damage values
  - Better naming
 */

typedef s64 DamageValue;

typedef enum {
    DMG_KIND_FIRE,
    DMG_KIND_LIGHTNING,
    DMG_KIND_COUNT,
} DamageKind;

// TODO: rename this struct
typedef struct {
    DamageValue values[DMG_KIND_COUNT]; // Use DamageKind as index
} DamageTypes;

typedef struct {
    DamageTypes low_roll;
    DamageTypes high_roll;
} DamageRange;

typedef struct {
    DamageTypes types;
    DamageTypes penetration;
} DamageInstance;

typedef struct {
    DamageTypes additive_modifiers;
    DamageTypes multiplicative_modifiers;
} DamageValueModifiers;

struct Entity;

static inline DamageValue *get_damage_reference_of_type(DamageTypes *damages, DamageKind type)
{
    ssize index = (ssize)type;
    ASSERT((index >= 0) && (index < DMG_KIND_COUNT));

    DamageValue *result = &damages->values[index];

    return result;
}

static inline DamageValue get_damage_value_of_type(DamageTypes damages, DamageKind type)
{
    DamageValue result = *get_damage_reference_of_type(&damages, type);
    return result;
}

static inline DamageValue calculate_damage_sum(DamageTypes damage)
{
    DamageValue result = 0;

    for (DamageKind kind = 0; kind < DMG_KIND_COUNT; ++kind) {
	result += get_damage_value_of_type(damage, kind);
    }

    return result;
}

static inline DamageValue calculate_damage_of_type_after_resistance(DamageInstance damage,
    DamageTypes resistances, DamageKind dmg_type)
{
    DamageValue damage_amount = get_damage_value_of_type(damage.types, dmg_type);
    DamageValue resistance = get_damage_value_of_type(resistances, dmg_type);

    resistance -= get_damage_value_of_type(damage.penetration, dmg_type);

    // TODO: round up or down?
    DamageValue result = (DamageValue)((f64)damage_amount * (1.0 - (f64)resistance / 100.0));

    return result;
}

static inline DamageInstance calculate_damage_received(DamageTypes resistances, DamageInstance damage)
{
    DamageInstance result = {0};

    for (DamageKind dmg_kind = 0; dmg_kind < DMG_KIND_COUNT; ++dmg_kind) {
        result.types.values[dmg_kind] =
	    calculate_damage_of_type_after_resistance(damage, resistances, dmg_kind);
    }

    return result;
}

static inline void set_damage_value_of_type(DamageTypes *damages, DamageKind kind, DamageValue new_value)
{
    DamageValue *old_value = get_damage_reference_of_type(damages, kind);
    *old_value = new_value;
}

static inline DamageTypes calculate_damage_dealt_from_range(DamageRange damage_range)
{
    // TODO: make caster stats influence damage roll
    DamageTypes result = {0};

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
	DamageValue low = get_damage_value_of_type(damage_range.low_roll, type);
	DamageValue high = get_damage_value_of_type(damage_range.high_roll, type);

	DamageValue roll = rng_s64(low, high);
	set_damage_value_of_type(&result, type, roll);
    }

    return result;
}

static inline void set_damage_range_for_type(DamageRange *range, DamageKind type,
    DamageValue low, DamageValue high)
{
    set_damage_value_of_type(&range->low_roll, type, low);
    set_damage_value_of_type(&range->high_roll, type, high);
}

DamageTypes calculate_damage_after_boosts(DamageTypes damage, struct Entity *entity);

#endif //DAMAGE_H
