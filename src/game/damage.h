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
 */

struct ItemSystem;
struct Entity;

typedef s64 DamageValue;

typedef enum {
    DMG_TYPE_FIRE,
    DMG_TYPE_LIGHTNING,
    DMG_TYPE_COUNT,
} DamageType;

// NOTE: the ordering of these affects the order of calculations
// TODO: additive percentage bonuses
// TODO: unique modifiers that aren't just additive
typedef enum {
    NUMERIC_MOD_FLAT_ADDITIVE,
    NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE,
    NUMERIC_MOD_TYPE_COUNT,
} NumericModifierType;

// TODO: move elsewhere
// NOTE: these are broad categories of all mod types, not specific modifier types on items for example
typedef enum ModifierKind {
    MOD_CATEGORY_DAMAGE,
    MOD_CATEGORY_RESISTANCE,
} ModifierCategory;

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
					     ModifierCategory mod_category, struct ItemSystem *item_mgr);
DamageValues   calculate_damage_dealt(DamageValues base_damage, struct Entity *entity,
				      struct ItemSystem *item_mgr);
DamageValues   calculate_resistances_after_boosts(struct Entity *entity, struct ItemSystem *item_mgr);
DamageValues   calculate_damage_received(DamageValues resistances, DamageInstance damage);
void	       set_damage_value_for_type(DamageValues *damages, DamageType kind, DamageValue new_value);
DamageValues   roll_damage_in_range(DamageRange range);
DamageValue    calculate_damage_sum(DamageValues damage);
DamageValue    get_damage_value_for_type(DamageValues damages, DamageType type);

// TODO: make this return by value instead
void	       set_damage_range_for_type(DamageRange *range, DamageType type, DamageValue low,
					 DamageValue high);

#endif //DAMAGE_H
