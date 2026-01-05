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
    DMG_CALC_PHASE_ADDITIVE,       // Flat bonuses
    DMG_CALC_PHASE_MULTIPLICATIVE, // Multiplicative percentage bonuses
    DMG_CALC_PHASE_COUNT,
} DamageCalculationPhase;

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

DamageValue    get_damage_value_of_type(DamageTypes damages, DamageKind type);
DamageValue    calculate_damage_sum(DamageTypes damage);
DamageInstance calculate_damage_received(DamageTypes resistances, DamageInstance damage);
void	       set_damage_value_of_type(DamageTypes *damages, DamageKind kind, DamageValue new_value);
DamageTypes    calculate_damage_dealt_from_damage_range(DamageRange damage_range);
void	       set_damage_range_for_type(DamageRange *range, DamageKind type, DamageValue low,
					 DamageValue high);
DamageTypes    calculate_total_flat_damage_bonuses(Entity *entity, ItemManager *item_mgr);
DamageTypes    calculate_damage_after_boosts(DamageTypes damage, struct Entity *entity,
					     struct ItemManager *item_mgr);
DamageTypes    calculate_resistances_after_boosts(DamageTypes base_resistances, struct Entity *entity,
						  struct ItemManager *item_mgr);
String damage_type_to_string(DamageKind type);

#endif //DAMAGE_H
