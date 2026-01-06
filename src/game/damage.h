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
    NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE,
    NUMERIC_MOD_TYPE_COUNT,
} NumericModifierType;

// TODO: move elsewhere
// NOTE: these are broad categories of all mod types, not specific modifier types on items for example
typedef enum ModifierKind {
    MODIFIER_DAMAGE,
    MODIFIER_RESISTANCE,
} ModifierKind; // TODO: rename to modifiercategory

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

DamageValues   get_numeric_modifiers_of_type(struct Entity *entity, NumericModifierType type,
					     ModifierKind mod_category, struct ItemManager *item_mgr);
DamageValues   calculate_damage_dealt(DamageValues base_damage, struct Entity *entity,
				      struct ItemManager *item_mgr);
DamageValues   calculate_resistances_after_boosts(struct Entity *entity, struct ItemManager *item_mgr);
DamageValues   calculate_damage_received(DamageValues resistances, DamageInstance damage);
void	       set_damage_value_of_type(DamageValues *damages, DamageKind kind, DamageValue new_value);
DamageValues   roll_damage_in_range(DamageRange range);
DamageValue    calculate_damage_sum(DamageValues damage);
DamageValue    get_damage_value_for_type(DamageValues damages, DamageKind type);

// TODO: make this return by value instead
void	       set_damage_range_for_type(DamageRange *range, DamageKind type, DamageValue low,
					 DamageValue high);

#endif //DAMAGE_H
