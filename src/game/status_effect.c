#include "status_effect.h"
#include "base/utils.h"
#include "entity/entity_system.h"
#include "stats.h"
#include "components/component.h"

typedef enum {
    EFFECT_PROP_STAT_MODIFIER	= FLAG(0),
    EFFECT_PROP_STACKABLE	= FLAG(1),
} StatusEffectProperty;

typedef struct {
    StatusEffectProperty properties;
    f32 base_duration;

    /* Property specific fields */
    Modifier stat_modifier;
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

static StatusEffect status_effect_chilled(void)
{
    StatusEffect result = {0};

    result.base_duration = 10.0f;
    result.properties = EFFECT_PROP_STAT_MODIFIER;
    result.stat_modifier = create_modifier(STAT_ACTION_SPEED, -75, NUMERIC_MOD_FLAT_ADDITIVE);

    return result;
}

void initialize_status_effect_system(void)
{
    g_status_effects[STATUS_EFFECT_CHILLED] = status_effect_chilled();
}

void apply_status_effect(StatusEffectComponent *comp, StatusEffectID effect_id)
{
    StatusEffect *effect = get_status_effect_by_id(effect_id);
    ASSERT(effect->base_duration > 0.0f);

    comp->active_effects_bitset |= FLAG(effect_id);

    // Add to effect list
    if (status_effect_is_stackable(effect_id)) {
	StatusEffectStack *stack = &comp->stackable_effects[effect_id];

	if (stack->effect_count < ARRAY_COUNT(stack->effects)) {
	    StatusEffectInstance instance = {effect->base_duration};
	    stack->effects[stack->effect_count++] = instance;
	} else {
	    UNIMPLEMENTED;
	}
    } else {
	StatusEffectInstance *instance = &comp->non_stackable_effects[effect_id];

	instance->time_remaining = effect->base_duration;
    }

    // Perform effect specific actions?
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

static StatusEffectCallbackResult update_status_effects_callback(StatusEffectCallbackParams params,
    void *user_data)
{
    StatusEffectCallbackResult result = STATUS_EFFECT_CALLBACK_PROCEED;

    f32 dt = *(f32 *)user_data;

    params.instance->time_remaining -= dt;

    if (params.instance->time_remaining <= 0.0f) {
	result = STATUS_EFFECT_CALLBACK_DELETE_CURRENT;
    }

    return result;
}

void update_status_effects(StatusEffectComponent *status_effects, f32 dt)
{
    for_each_active_status_effect(status_effects, update_status_effects_callback, &dt);
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
    switch (effect_id) {
	case STATUS_EFFECT_CHILLED: return str_lit("Chilled");
	case STATUS_EFFECT_COUNT:   ASSERT(0);
    }

    ASSERT(0);
    return (String){0};
}

b32 has_status_effect(struct StatusEffectComponent *comp, StatusEffectID id)
{
    ASSERT(comp);
    b32 result = (comp->active_effects_bitset & FLAG(id)) != 0;

    return result;
}

b32 status_effect_is_stackable(StatusEffectID id)
{
    StatusEffect *effect = get_status_effect_by_id(id);

    b32 result = effect_has_prop(effect, EFFECT_PROP_STACKABLE);

    return result;
}

void for_each_active_status_effect(StatusEffectComponent *comp, StatusEffectCallback fn, void *data)
{
    StatusEffectCallbackParams params = {0};
    params.status_effects_component = comp;

    for (StatusEffectID id = 0; id < STATUS_EFFECT_COUNT; ++id) {
	if (has_status_effect(comp, id)) {
	    params.status_effect_id = id;

	    if (status_effect_is_stackable(id)) {
		StatusEffectStack *stack = &comp->stackable_effects[id];
		ASSERT(stack->effect_count >= 0);

		for (ssize i = 0; i < stack->effect_count; ++i) {
		    StatusEffectInstance *instance = &stack->effects[i];
		    params.instance = instance;

		    StatusEffectCallbackResult result = fn(params, data);

		    if (result == STATUS_EFFECT_CALLBACK_DELETE_CURRENT) {
			*instance = stack->effects[stack->effect_count - 1];
			--stack->effect_count;
			--i;

			if (stack->effect_count == 0) {
			    remove_status_effects_of_type(comp, id);
			}
		    }
		}
	    } else {
		params.instance = &comp->non_stackable_effects[id];

		StatusEffectCallbackResult result = fn(params, data);

		if (result == STATUS_EFFECT_CALLBACK_DELETE_CURRENT) {
		    remove_status_effects_of_type(comp, id);
		}
	    }
	}
    }
}
