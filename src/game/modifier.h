#ifndef MODIFIER_H
#define MODIFIER_H

#include "base/format.h"
#include "damage.h"

typedef enum {
    MODIFIER_DAMAGE = (1 << 0),
    MODIFIER_RESISTANCE = (1 << 1),
} ModifierKind;

typedef struct {
    NumericModifierType applied_during_phase;
    DamageKind affected_damage_kind;
    DamageValue value;
} DamageModifier;

typedef struct {
    DamageKind type;
    DamageValue value;
} ResistanceModifier;

typedef struct {
    ModifierKind kind;

    union {
        DamageModifier damage_modifier;
        ResistanceModifier resistance_modifier;
    } as;
} StatModifier;

static inline StatModifier create_damage_modifier(NumericModifierType boost_kind,
    DamageKind type, DamageValue value)
{
    StatModifier result = {0};
    result.kind = MODIFIER_DAMAGE;

    DamageModifier *dmg_mod = &result.as.damage_modifier;
    dmg_mod->applied_during_phase = boost_kind;
    dmg_mod->affected_damage_kind = type;
    dmg_mod->value = value;

    return result;
}

static inline String modifier_to_string(StatModifier modifier, Allocator alloc)
{
    String result = {0};

    switch (modifier.kind) {
	case MODIFIER_DAMAGE: {
	    DamageModifier dmg_mod = modifier.as.damage_modifier;
	    result = ssize_to_string(dmg_mod.value, alloc);

	    if (dmg_mod.applied_during_phase == NUMERIC_MOD_FLAT_ADDED) {
		result = str_concat(str_lit("+"), result, alloc);
	    } else if (dmg_mod.applied_during_phase == NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE) {
		result = str_concat(result, str_lit("% increased"), alloc);
	    } else {
		ASSERT(0);
	    }

	    result = str_concat(result, str_lit(" "), alloc);
	    result = str_concat(result, damage_type_to_string(dmg_mod.affected_damage_kind), alloc);
	} break;

	case MODIFIER_RESISTANCE: {
	    ASSERT(0);
	} break;
    }

    return result;
}

#endif //MODIFIER_H
