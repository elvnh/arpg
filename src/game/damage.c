#include "damage.h"
#include "base/utils.h"
#include "components/status_effect.h"
#include "entity_system.h"

// NOTE: the ordering of these affects the order of calculations
typedef enum {
    DMG_CALC_PHASE_ADDITIVE,
    DMG_CALC_PHASE_COUNT,
} DamageCalculationPhase;

static DamageTypes damage_types_add(DamageTypes lhs, DamageTypes rhs)
{
    DamageTypes result = lhs;

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
        DamageValue *lhs_ptr = get_damage_reference_of_type(&result, type);
        DamageValue rhs_value = get_damage_value_of_type(rhs, type);

        *lhs_ptr += rhs_value;
    }

    return result;
}

static DamageTypes apply_damage_value_status_effects(DamageTypes value, struct Entity *entity,
    StatusEffectKind interesting_effects, DamageCalculationPhase phase)
{
    DamageTypes result = value;

    StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);

    if (!status_effects) {
        return result;
    }

    for (s32 i = 0; i < status_effects->effect_count; ++i) {
        StatusEffect *effect = &status_effects->effects[i];
        b32 is_interesting = (effect->kind & interesting_effects) != 0;

        if (is_interesting) {
            switch (effect->kind) {
                case STATUS_EFFECT_DAMAGE_MODIFIER: {
                    if (phase == DMG_CALC_PHASE_ADDITIVE) {
                        result = damage_types_add(result, effect->as.damage_modifiers.additive_modifiers);
                    } else {
                        ASSERT(0);
                    }
                } break;

                case STATUS_EFFECT_RESISTANCE_MODIFIER: {
                    if (phase == DMG_CALC_PHASE_ADDITIVE) {
                        result = damage_types_add(result, effect->as.resistance_modifiers);
                    } else {
                        ASSERT(0);
                    }
                } break;

                default: break;
            }
        }
    }

    return result;
}

DamageTypes calculate_damage_after_boosts(DamageTypes damage, struct Entity *entity)
{
    DamageTypes result = damage;

    for (DamageCalculationPhase phase = 0; phase < DMG_CALC_PHASE_COUNT; ++phase) {
        result = apply_damage_value_status_effects(result, entity, STATUS_EFFECT_DAMAGE_MODIFIER, phase);
    }

    return result;
}

// TODO: make this take only entity as parameter
DamageTypes calculate_resistances_after_boosts(DamageTypes base_resistances, struct Entity *entity)
{
    // NOTE: resistances only have an additive phase
    DamageTypes result = base_resistances;

    result = apply_damage_value_status_effects(result, entity, STATUS_EFFECT_RESISTANCE_MODIFIER,
        DMG_CALC_PHASE_ADDITIVE);

    return result;
}
