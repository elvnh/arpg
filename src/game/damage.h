#ifndef DAMAGE_H
#define DAMAGE_H

#include "health.h"
#include "base/utils.h"
#include "base/random.h"
#include "base/string8.h"

/*
  TODO:
  - Better way of setting resistance/damage values
  - Better naming
  - Clean up .c-file function ordering
  - Clean up the way modifiers are accumulated and applied
 */

struct ItemSystem;
struct Entity;

typedef s64 DamageValue;

typedef enum {
    DMG_TYPE_FIRE,
    DMG_TYPE_LIGHTNING,
    DMG_TYPE_COUNT,
} DamageType;

// TODO: unique modifiers that aren't just numeric
typedef enum {
    NUMERIC_MOD_FLAT_ADDITIVE,
    NUMERIC_MOD_ADDITIVE_PERCENTAGE,
    NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE,
    NUMERIC_MOD_TYPE_COUNT,
} NumericModifierType;

// TODO: move elsewhere
// TODO: once stats are implemented, stat enum could be used instead
// Represents which stat is affected by a modifier
typedef enum {
    MOD_TARGET_DAMAGE,
    MOD_TARGET_RESISTANCE,
} ModifierTarget;

// TODO: use this in modifiers too
typedef struct {
    DamageValue value;
    DamageType type;
} TypedDamageValue;

typedef struct {
    DamageValue values[DMG_TYPE_COUNT]; // Use DamageKind as index
} DamageValues;

typedef struct {
    DamageValues low_roll;
    DamageValues high_roll;
} DamageRange;

typedef struct {
    DamageValues types;
    DamageValues penetration;
} DamageInstance;

DamageValues   get_numeric_modifiers_of_type(struct Entity *entity, NumericModifierType type,
					     ModifierTarget mod_category, struct ItemSystem *item_sys);
DamageValues   calculate_damage_dealt(DamageValues base_damage, struct Entity *entity,
				      struct ItemSystem *item_sys);
DamageValues   calculate_resistances_after_boosts(struct Entity *entity, struct ItemSystem *item_sys);
DamageValues   calculate_damage_received(DamageValues resistances, DamageInstance damage);
void	       set_damage_value_for_type(DamageValues *damages, DamageType kind, DamageValue new_value);
DamageValues   roll_damage_in_range(DamageRange range);
DamageValue    calculate_damage_sum(DamageValues damage);
DamageValue    get_damage_value_for_type(DamageValues damages, DamageType type);

// TODO: make this return by value instead
void	       set_damage_range_for_type(DamageRange *range, DamageType type, DamageValue low,
					 DamageValue high);

static inline String damage_type_to_string(DamageType type)
{
    switch (type) {
	case DMG_TYPE_FIRE: return str_lit("Fire");
	case DMG_TYPE_LIGHTNING: return str_lit("Lightning");
	case DMG_TYPE_COUNT: ASSERT(0);
    }

    ASSERT(0);

    return (String){0};
}

#endif //DAMAGE_H
