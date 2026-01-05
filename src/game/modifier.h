#ifndef MODIFIER_H
#define MODIFIER_H

#include "base/utils.h"
#include "damage.h"

typedef enum {
    MODIFIER_DAMAGE = (1 << 0),
    MODIFIER_RESISTANCE = (1 << 1),
} ModifierKind;

typedef struct {
    DamageCalculationPhase applied_during_phase;
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
} Modifier;

static inline Modifier create_damage_modifier(DamageCalculationPhase boost_kind,
    DamageKind type, DamageValue value)
{
    Modifier result = {0};
    result.kind = MODIFIER_DAMAGE;

    DamageModifier *dmg_mod = &result.as.damage_modifier;
    dmg_mod->applied_during_phase = boost_kind;
    dmg_mod->affected_damage_kind = type;
    dmg_mod->value = value;

    return result;
}

#endif //MODIFIER_H
