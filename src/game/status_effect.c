#include "status_effect.h"
#include "entity_system.h"
#include "stats.h"
#include "components/component.h"

typedef enum {
    EFFECT_PROP_STAT_MODIFIER	= (1 << 0),
    EFFECT_PROP_STACKABLE	= (1 << 1),
} StatusEffectProperty;

typedef struct {
    StatusEffectProperty properties;
    f32 base_duration;

    Modifier stat_modifier;
    //s32 max_stack_size;
} StatusEffect;

StatusEffect g_status_effects[STATUS_EFFECT_COUNT];

static b32 effect_has_prop(StatusEffect *effect, StatusEffectProperty prop)
{
    b32 result = (effect->properties & prop) != 0;

    return result;
}

static StatusEffect *get_status_effect_by_id(StatusEffectID id)
{
    ASSERT(id >= 0);
    ASSERT(id < STATUS_EFFECT_COUNT);

    StatusEffect *result = &g_status_effects[id];

    return result;
}

static StatusEffect status_effect_chilled()
{
    StatusEffect result = {0};

    result.base_duration = 10.0f;

    result.properties = EFFECT_PROP_STAT_MODIFIER;

    // TODO: reduce action speed, ie both cast speed and movement speed
    result.stat_modifier = create_modifier(STAT_CAST_SPEED, -75, NUMERIC_MOD_FLAT_ADDITIVE);

    return result;
}

void initialize_status_effect_system()
{
    g_status_effects[STATUS_EFFECT_CHILLED] = status_effect_chilled();
}

void apply_status_effect(StatusEffectComponent *comp, StatusEffectID effect_id)
{
    StatusEffect *effect = get_status_effect_by_id(effect_id);
    ASSERT(effect->base_duration > 0.0f);

    comp->active_effects_bitset |= FLAG(effect_id);

    // Add to effect list
    if (effect_has_prop(effect, EFFECT_PROP_STACKABLE)) {
	StatusEffectStack *stack = &comp->stackable_effects[effect_id];

	if (stack->effect_count < ARRAY_COUNT(stack->effects)) {
	    StatusEffectInstance instance = {effect->base_duration};
	    stack->effects[stack->effect_count++] = instance;
	} else {
	    ASSERT(0 && "TODO");
	}
    } else {
	StatusEffectInstance *instance = &comp->non_stackable_effects[effect_id];

	instance->time_remaining = effect->base_duration;
    }

    // Perform effect specific actions
}

void try_apply_status_effect_to_entity(Entity *entity, StatusEffectID effect_id)
{
    StatusEffectComponent *comp = es_get_component(entity, StatusEffectComponent);

    if (!comp) {
	return;
    }

    apply_status_effect(comp, effect_id);
}

static void remove_status_effects_of_type(StatusEffectComponent *status_effects, StatusEffectID type)
{
    u64 bit = FLAG(type);
    status_effects->active_effects_bitset &= ~bit;
}

static void update_status_effects_stack(StatusEffectComponent *comp, StatusEffectStack *stack,
    StatusEffectID id, f32 dt)
{
    ASSERT(stack->effect_count > 0 &&
	"If effect instances of this type is zero, "
	"the bit in the active effects bitset shouldn't be set");

    for (ssize i = 0; i < stack->effect_count; ++i) {
	StatusEffectInstance *instance = &stack->effects[i];

	instance->time_remaining -= dt;

	if (instance->time_remaining <= 0.0f) {
	    *instance = stack->effects[stack->effect_count - 1];
	    --stack->effect_count;
	    --i;

	    if (stack->effect_count == 0) {
		remove_status_effects_of_type(comp, id);
	    }
	}
    }
}

void update_status_effects(StatusEffectComponent *status_effects, f32 dt)
{
    for (StatusEffectID id = 0; id < STATUS_EFFECT_COUNT; ++id) {
	u64 bit = FLAG(id);

	if (status_effects->active_effects_bitset & bit) {
	    StatusEffect *effect = get_status_effect_by_id(id);

	    if (effect_has_prop(effect, EFFECT_PROP_STACKABLE)) {
		StatusEffectStack *stack = &status_effects->stackable_effects[id];

		update_status_effects_stack(status_effects, stack, id, dt);
	    } else {
		StatusEffectInstance *instance = &status_effects->non_stackable_effects[id];

		instance->time_remaining -= dt;

		if (instance->time_remaining <= 0.0f) {
		    remove_status_effects_of_type(status_effects, id);
		}
	    }
	}
    }
}

b32 status_effect_modifies_stat(StatusEffectID effect_id, Stat stat, NumericModifierType mod_type)
{
    b32 result = false;

    StatusEffect *effect = get_status_effect_by_id(effect_id);

    if (effect_has_prop(effect, EFFECT_PROP_STAT_MODIFIER)) {
	Modifier mod = effect->stat_modifier;
	result = (mod.target_stat == stat) && (mod.modifier_type == mod_type);
    }

    return result;
}

Modifier get_status_effect_stat_modifier(StatusEffectID effect_id)
{
    StatusEffect *effect = get_status_effect_by_id(effect_id);
    ASSERT(effect_has_prop(effect, EFFECT_PROP_STAT_MODIFIER));

    Modifier result = effect->stat_modifier;

    return result;
}

String status_effect_to_string(StatusEffectID effect_id)
{
    (void)effect_id;

    return str_lit("<unimplemented>");
}

b32 has_status_effect(struct StatusEffectComponent *comp, StatusEffectID id)
{
    b32 result = (comp->active_effects_bitset & FLAG(id)) != 0;

    return result;
}

b32 status_effect_is_stackable(StatusEffectID id)
{
    StatusEffect *effect = get_status_effect_by_id(id);

    b32 result = effect_has_prop(effect, EFFECT_PROP_STACKABLE);

    return result;
}

// on tick
// on apply
// on remove
// ?
