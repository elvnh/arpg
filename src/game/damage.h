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
 */

struct ItemManager;
struct Entity;

typedef s64 DamageValue;

typedef enum {
    DMG_KIND_FIRE,
    DMG_KIND_LIGHTNING,
    DMG_KIND_COUNT,
} DamageKind;

// NOTE: the ordering of these affects the order of calculations
// TODO: additive percentage bonuses
typedef enum {
    NUMERIC_MOD_FLAT_ADDED,
    NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE, // Multiplicative percentage bonuses
    NUMERIC_MOD_TYPE_COUNT,
} NumericModifierType;

typedef struct {
    DamageValue values[DMG_KIND_COUNT]; // Use DamageKind as index
} DamageValues;

typedef struct {
    DamageValues low_roll;
    DamageValues high_roll;
} DamageRange;

typedef struct {
    DamageValues types;
    DamageValues penetration;
} DamageInstance;

DamageValue    get_damage_value_of_type(DamageValues damages, DamageKind type);
DamageValue    calculate_damage_sum(DamageValues damage);
DamageInstance calculate_damage_received(DamageValues resistances, DamageInstance damage);
void	       set_damage_value_of_type(DamageValues *damages, DamageKind kind, DamageValue new_value);
DamageValues    calculate_damage_dealt_from_damage_range(DamageRange damage_range);
void	       set_damage_range_for_type(DamageRange *range, DamageKind type, DamageValue low,
					 DamageValue high);
DamageValues    calculate_damage_after_boosts(DamageValues damage, struct Entity *entity,
					     struct ItemManager *item_mgr);
DamageValues    calculate_resistances_after_boosts(DamageValues base_resistances, struct Entity *entity,
						  struct ItemManager *item_mgr);
String damage_type_to_string(DamageKind type);

#endif //DAMAGE_H
