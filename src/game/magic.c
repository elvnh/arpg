#include "magic.h"
#include "asset.h"
#include "damage.h"
#include "game/collision.h"
#include "world.h"
#include "component.h"
#include "asset.h"
#include "entity_system.h"

typedef struct {
    Damage base_damage;
    TextureHandle texture;
    Vector2 sprite_size;
    /*
      base dmg
      proj? channeling?
      base speed
      sprite
     */
} Spell;

static Spell g_spells[SPELL_COUNT] = {0};

// TODO: make it easier to define spells
void magic_initialize(const AssetList *asset_list)
{
    Damage fireball_damage = {0};
    fireball_damage.types.values[DMG_KIND_FIRE] = 1;

    g_spells[SPELL_FIREBALL].texture = asset_list->fireball_texture;
    g_spells[SPELL_FIREBALL].base_damage = fireball_damage;
    g_spells[SPELL_FIREBALL].sprite_size = v2(32, 32);
}

static const Spell *get_spell_by_id(SpellID id)
{
    ASSERT(id >= 0);
    ASSERT(id < SPELL_COUNT);

    return &g_spells[id];
}

void magic_cast_spell(struct World *world, SpellID id, struct Entity *caster, Vector2 position, Vector2 velocity)
{
    EntityID caster_id = es_get_id_of_entity(&world->entity_system, caster);

    const Spell *spell = get_spell_by_id(id);
    Damage damage_dealt = spell->base_damage;

    EntityWithID spell_entity_with_id = es_spawn_entity(&world->entity_system, caster->faction);
    Entity *spell_entity = spell_entity_with_id.entity;
    spell_entity->faction = caster->faction;

    ColliderComponent *collider =  es_add_component(spell_entity, ColliderComponent);
    collider->size = v2(32, 32);

    add_damage_collision_effect(collider, damage_dealt, OBJECT_KIND_ENTITIES, false);
    add_bounce_collision_effect(collider, OBJECT_KIND_TILES, false);
    //add_die_collision_effect(collider, (OBJECT_KIND_ENTITIES | OBJECT_KIND_TILES), false);
    add_passthrough_collision_effect(collider, OBJECT_KIND_ENTITIES);

    SpriteComponent *sprite = es_add_component(spell_entity, SpriteComponent);
    sprite->texture = spell->texture;
    sprite->size = spell->sprite_size;
    // TODO: particle spawner

    spell_entity->position = position;
    spell_entity->velocity = velocity;
}
