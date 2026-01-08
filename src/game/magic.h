#ifndef MAGIC_H
#define MAGIC_H

#include "base/vector.h"
#include "asset.h"
#include "components/particle.h"
#include "damage.h"
#include "components/collider.h"
#include "sprite.h"

struct SpellCasterComponent;

typedef enum {
    SPELL_FIREBALL,
    SPELL_SPARK,
    SPELL_COUNT,
} SpellID;

typedef enum {
    SPELL_PROP_PROJECTILE = (1 << 0),
    SPELL_PROP_DAMAGING = (1 << 1),
    SPELL_PROP_SPRITE = (1 << 2),
    SPELL_PROP_BOUNCE_ON_TILES = (1 << 3),
    SPELL_PROP_DIE_ON_WALL_COLLISION = (1 << 4),
    SPELL_PROP_DIE_ON_ENTITY_COLLISION = (1 << 5),
    SPELL_PROP_LIFETIME = (1 << 6),
    SPELL_PROP_PARTICLE_SPAWNER = (1 << 7),
} SpellProperties;

typedef struct {
    SpellProperties properties;

    /* Flag specific fields */
    Sprite sprite;

    struct {
	// TODO: pack base_damage and penetration_values into struct
	DamageRange base_damage;
	DamageValues penetration_values;
	CollisionRetriggerBehaviour retrigger_behaviour;
    } damaging;

    struct {
	f32 projectile_speed;
	Vector2 collider_size;
	s32 extra_projectile_count;
    } projectile;

    f32 lifetime;

    ParticleSpawnerConfig particle_spawner;
    /*
      damaging
      healing
      spell kind - projectile, channeling etc

      Damage
      damage roll

      Projectile
      projectile speed


      base dmg
      proj? channeling?
      base speed
      sprite
     */
} Spell;

typedef struct SpellArray {
    Spell spells[SPELL_COUNT];
} SpellArray;

struct Entity;
struct World;

void magic_initialize(SpellArray *spells);
void magic_cast_spell(struct World *world, SpellID id, struct Entity *caster, Vector2 pos, Vector2 dir);
void magic_set_global_spell_array(SpellArray *spells);
void magic_add_to_spellbook(struct SpellCasterComponent *spellcaster, SpellID id);
String spell_type_to_string(SpellID id);
SpellID get_spell_at_spellbook_index(struct SpellCasterComponent *spellcaster, ssize index);

#endif //MAGIC_H
