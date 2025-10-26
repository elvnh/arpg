#include "magic.h"

#include "base/utils.h"
#include "game/collision.h"
#include "game/damage.h"
#include "world.h"
#include "component.h"
#include "asset.h"
#include "entity_system.h"

static SpellArray *g_spells;

static Spell spell_fireball(const AssetList *asset_list)
{
    Spell spell = {0};

    spell.properties = SPELL_PROP_PROJECTILE | SPELL_PROP_PROJECTILE | SPELL_PROP_DAMAGING
	| SPELL_PROP_SPRITE;

    spell.sprite.texture = asset_list->fireball_texture;
    spell.sprite.sprite_size = v2(32, 32);

    spell.projectile.projectile_speed = 100.0f;
    spell.projectile.collider_size = v2(32, 32);

    Damage damage = {0};
    damage.types.values[DMG_KIND_FIRE] = 10;

    spell.damaging.base_damage = damage;

    return spell;
}

static b32 spell_has_prop(const Spell *spell, SpellProperties prop)
{
    b32 result = (spell->properties & prop) != 0;

    return result;
}

// TODO: make it easier to define spells
void magic_initialize(SpellArray *spells, const struct AssetList *asset_list)
{
    spells->spells[SPELL_FIREBALL] = spell_fireball(asset_list);

    magic_set_global_spell_array(spells);
}

static const Spell *get_spell_by_id(SpellID id)
{
    ASSERT(id >= 0);
    ASSERT(id < SPELL_COUNT);

    return &g_spells->spells[id];
}

void magic_cast_spell(struct World *world, SpellID id, struct Entity *caster, Vector2 position,
    Vector2 direction)
{
    //ASSERT(v2_mag(direction) == 1.0f);

    //EntityID caster_id = es_get_id_of_entity(&world->entity_system, caster);

    EntityWithID spell_entity_with_id = es_spawn_entity(&world->entity_system, caster->faction);
    Entity *spell_entity = spell_entity_with_id.entity;
    spell_entity->position = position;

    const Spell *spell = get_spell_by_id(id);

    if (spell_has_prop(spell, SPELL_PROP_SPRITE)) {
	SpriteComponent *sprite = es_get_or_add_component(spell_entity, SpriteComponent);
	sprite->texture = spell->sprite.texture;
	sprite->size = spell->sprite.sprite_size;
    }

    if (spell_has_prop(spell, SPELL_PROP_PROJECTILE)) {
	ColliderComponent *collider = es_get_or_add_component(spell_entity, ColliderComponent);
	collider->size = spell->projectile.collider_size;

	spell_entity->velocity = v2_mul_s(direction, spell->projectile.projectile_speed);
    }

#define SPELL_COLLISION_EFFECT_PROPERTIES (SPELL_PROP_BOUNCE_ON_TILES | SPELL_PROP_DAMAGING)

    // NOTE: for now, either die on collision with walls unless bounce flag is set
    // TODO: more dynamic behaviour
    if (spell->properties & SPELL_COLLISION_EFFECT_PROPERTIES) {
	ColliderComponent *collider = es_get_or_add_component(spell_entity, ColliderComponent);

        if (spell_has_prop(spell, SPELL_PROP_BOUNCE_ON_TILES)) {
            OnCollisionEffect *effect = get_or_add_collide_effect(collider, ON_COLLIDE_BOUNCE);
            effect->affects_object_kinds |= OBJECT_KIND_TILES;
        }

        if (spell_has_prop(spell, SPELL_PROP_DAMAGING)) {
            OnCollisionEffect *effect = get_or_add_collide_effect(collider, ON_COLLIDE_DEAL_DAMAGE);
            effect->affects_object_kinds |= OBJECT_KIND_ENTITIES;
            effect->as.deal_damage.damage = spell->damaging.base_damage;
        }

        if (spell_has_prop(spell, SPELL_PROP_DIE_ON_ENTITY_COLLISION)) {
            OnCollisionEffect *effect = get_or_add_collide_effect(collider, ON_COLLIDE_DIE);
            effect->affects_object_kinds |= OBJECT_KIND_ENTITIES;
        }

        if (spell_has_prop(spell, SPELL_PROP_DIE_ON_WALL_COLLISION)) {
            OnCollisionEffect *effect = get_or_add_collide_effect(collider, ON_COLLIDE_DIE);
            effect->affects_object_kinds |= OBJECT_KIND_TILES;
        }
    }




}

void magic_set_global_spell_array(SpellArray *spells)
{
    g_spells = spells;
}
