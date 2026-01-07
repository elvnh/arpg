#include "magic.h"
#include "base/utils.h"
#include "components/collider.h"
#include "game/collision.h"
#include "game/damage.h"
#include "world.h"
#include "components/component.h"
#include "asset.h"
#include "entity_system.h"
#include "asset_table.h"

static SpellArray *g_spells;

static Spell spell_fireball()
{
    Spell spell = {0};

    spell.properties = SPELL_PROP_PROJECTILE | SPELL_PROP_PROJECTILE | SPELL_PROP_DAMAGING
	| SPELL_PROP_SPRITE | SPELL_PROP_DIE_ON_WALL_COLLISION | SPELL_PROP_DIE_ON_ENTITY_COLLISION;

    spell.sprite.texture = get_asset_table()->fireball_texture;
    spell.sprite.size = v2(32, 32);
    spell.sprite.rotation_behaviour = SPRITE_ROTATE_BASED_ON_DIR;

    spell.projectile.projectile_speed = 100.0f;
    spell.projectile.collider_size = v2(32, 32);

    DamageRange damage_range = {0};
    set_damage_value_for_type(&damage_range.low_roll, DMG_TYPE_FIRE, 10);
    set_damage_value_for_type(&damage_range.high_roll, DMG_TYPE_FIRE, 20);

    spell.damaging.base_damage = damage_range;
    spell.damaging.retrigger_behaviour = COLL_RETRIGGER_NEVER;

    return spell;
}

static Spell spell_spark()
{
    Spell spell = {0};

    // TODO: erratic movement

    spell.properties = SPELL_PROP_PROJECTILE | SPELL_PROP_SPRITE | SPELL_PROP_DAMAGING
	| SPELL_PROP_BOUNCE_ON_TILES | SPELL_PROP_LIFETIME;

    spell.sprite.texture = get_asset_table()->spark_texture;
    spell.sprite.size = v2(32, 32);

    spell.projectile.projectile_speed = 500.0f;
    spell.projectile.collider_size = v2(32, 32);
    spell.projectile.extra_projectile_count = 5;

    spell.lifetime = 5.0f;

    set_damage_range_for_type(&spell.damaging.base_damage, DMG_TYPE_LIGHTNING, 1, 100);
    set_damage_value_for_type(&spell.damaging.penetration_values, DMG_TYPE_LIGHTNING, 20);
    spell.damaging.retrigger_behaviour = COLL_RETRIGGER_AFTER_NON_CONTACT;

    return spell;
}

static b32 spell_has_prop(const Spell *spell, SpellProperties prop)
{
    b32 result = (spell->properties & prop) != 0;

    return result;
}

// TODO: make it easier to define spells
void magic_initialize(SpellArray *spells)
{
    spells->spells[SPELL_FIREBALL] = spell_fireball();
    spells->spells[SPELL_SPARK] = spell_spark();

    magic_set_global_spell_array(spells);
}

static const Spell *get_spell_by_id(SpellID id)
{
    ASSERT(id >= 0);
    ASSERT(id < SPELL_COUNT);

    return &g_spells->spells[id];
}

static void cast_single_spell(struct World *world, const Spell *spell, struct Entity *caster,
    Vector2 pos, Vector2 dir)
{
    EntityWithID spell_entity_with_id = world_spawn_entity(world, caster->faction);
    Entity *spell_entity = spell_entity_with_id.entity;

    // TODO: remove, we set it at end of function
    spell_entity->position = pos;

    ColliderComponent *spell_collider = es_get_or_add_component(spell_entity, ColliderComponent);

    if (spell_has_prop(spell, SPELL_PROP_SPRITE)) {
	SpriteComponent *sprite_comp = es_get_or_add_component(spell_entity, SpriteComponent);

	sprite_comp->sprite = spell->sprite;
    }

    if (spell_has_prop(spell, SPELL_PROP_PROJECTILE)) {
	spell_collider->size = spell->projectile.collider_size;

	spell_entity->velocity = v2_mul_s(dir, spell->projectile.projectile_speed);
    }

    if (spell_has_prop(spell, SPELL_PROP_BOUNCE_ON_TILES)) {
	OnCollisionEffect *effect = get_or_add_collide_effect(spell_collider, ON_COLLIDE_BOUNCE);
	effect->affects_object_kinds |= OBJECT_KIND_TILES;
    }

    if (spell_has_prop(spell, SPELL_PROP_DAMAGING)) {
	OnCollisionEffect *effect = get_or_add_collide_effect(spell_collider, ON_COLLIDE_DEAL_DAMAGE);
	effect->affects_object_kinds |= OBJECT_KIND_ENTITIES;

	DamageValues damage_roll = roll_damage_in_range(spell->damaging.base_damage);
        DamageValues damage_after_boosts = calculate_damage_dealt(damage_roll, caster,
	    &world->item_manager);

        // TODO: create_damage_instance function
        DamageInstance damage = { damage_after_boosts, spell->damaging.penetration_values };

	effect->as.deal_damage.damage = damage;
	effect->retrigger_behaviour = spell->damaging.retrigger_behaviour;
    }

    if (spell_has_prop(spell, SPELL_PROP_DIE_ON_ENTITY_COLLISION)) {
	OnCollisionEffect *effect = get_or_add_collide_effect(spell_collider, ON_COLLIDE_DIE);
	effect->affects_object_kinds |= OBJECT_KIND_ENTITIES;
    }

    if (spell_has_prop(spell, SPELL_PROP_DIE_ON_WALL_COLLISION)) {
	OnCollisionEffect *effect = get_or_add_collide_effect(spell_collider, ON_COLLIDE_DIE);
	effect->affects_object_kinds |= OBJECT_KIND_TILES;
    }

    if (spell_has_prop(spell, SPELL_PROP_LIFETIME)) {
	ASSERT(spell->lifetime > 0.0f);

	LifetimeComponent *lifetime = es_add_component(spell_entity, LifetimeComponent);
	lifetime->time_to_live = spell->lifetime;
    }

    // Offset so that spells center is 'pos'
    Rectangle bounds = world_get_entity_bounding_box(spell_entity);
    spell_entity->position = v2_sub(pos, v2_div_s(bounds.size, 2.0f));
}

void magic_cast_spell(struct World *world, SpellID id, struct Entity *caster, Vector2 pos, Vector2 dir)
{
    const Spell *spell = get_spell_by_id(id);
    s32 spell_count = 1 + spell->projectile.extra_projectile_count;

    f32 current_angle = atan2f(dir.y, dir.x);
    f32 angle_step_size = 0.0f;

    if (spell_count >= 2) {
	// TODO: make it possible to configure cone size
	f32 cone = (PI / 2.0f);
	current_angle -= cone / 2.0f;
	angle_step_size = cone / ((f32)(spell_count - 1));
    }

    for (s32 i = 0; i < spell_count; ++i) {
	Vector2 current_dir = v2(cos_f32(current_angle), sin_f32(current_angle));
        Vector2 current_pos = v2_add(pos, v2_mul_s(dir, 20.0f));

	cast_single_spell(world, spell, caster, current_pos, current_dir);

	current_angle += angle_step_size;
    }
}

void magic_set_global_spell_array(SpellArray *spells)
{
    g_spells = spells;
}
