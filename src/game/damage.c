#include "damage.h"
#include "base/utils.h"
#include "components/status_effect.h"
#include "entity_system.h"

static DamageTypes damage_types_add(DamageTypes lhs, DamageTypes rhs)
{
    DamageTypes result = lhs;

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
        DamageValue *lhs_value = get_damage_reference_of_type(&result, type);
        DamageValue rhs_value = get_damage_value_of_type(rhs, type);

        *lhs_value += rhs_value;
    }

    return result;
}

static DamageTypes apply_additive_damage_value_modifiers(DamageTypes damage, Entity *entity)
{
    DamageTypes result = damage;

    if (es_has_component(entity, StatusEffectComponent)) {
        StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);

        for (s32 i = 0; i < status_effects->effect_count; ++i) {
            StatusEffect *effect = &status_effects->effects[i];

            switch (effect->kind) {
                case STATUS_EFFECT_DAMAGE_MODIFIER: {
                    result = damage_types_add(result, effect->as.damage_modifiers.additive_modifiers);
                } break;

                default: break;
            }
        }
    }

    return result;
}

// TODO: find a way to generalize different arithmetic operations on damage types
static DamageTypes apply_multiplicative_damage_value_modifiers(DamageTypes damage, Entity *entity)
{
    UNIMPLEMENTED;
    (void)damage;
    (void)entity;
}


DamageTypes calculate_damage_after_boosts(DamageTypes damage, struct Entity *entity)
{
    DamageTypes result = damage;

    result = apply_additive_damage_value_modifiers(result, entity);
    //result = apply_multiplicative_damage_value_modifiers(result, entity);

    return result;
}
