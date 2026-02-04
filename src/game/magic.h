#ifndef MAGIC_H
#define MAGIC_H

#include "base/vector.h"
#include "platform/asset.h"
#include "components/particle.h"
#include "damage.h"
#include "components/collider.h"
#include "light.h"
#include "status_effect.h"
#include "sprite.h"
#include "trigger.h"
#include "callback.h"

struct SpellCasterComponent;
struct Entity;
struct World;

/*
  TODO:
  - Scaling of spell aoe etc by caster stats
  - Don't define the SpellProperty bit values in declaration, do bitshift when needed
 */

typedef enum {
    SPELL_FIREBALL,
    SPELL_SPARK,
    SPELL_ICE_SHARD,
    SPELL_ICE_SHARD_TRIGGER,
    SPELL_BLIZZARD,
    SPELL_COUNT,
} SpellID;

typedef enum {
    SPELL_PROP_PROJECTILE                 = FLAG(0),
    SPELL_PROP_DAMAGING                   = FLAG(1),
    SPELL_PROP_SPRITE                     = FLAG(2),
    SPELL_PROP_BOUNCE_ON_TILES            = FLAG(3),
    SPELL_PROP_DIE_ON_WALL_COLLISION      = FLAG(4),
    SPELL_PROP_DIE_ON_ENTITY_COLLISION    = FLAG(5),
    SPELL_PROP_LIFETIME                   = FLAG(6),
    SPELL_PROP_PARTICLE_SPAWNER           = FLAG(7),
    SPELL_PROP_SPAWN_PARTICLES_ON_DEATH   = FLAG(8),
    SPELL_PROP_HOSTILE_COLLISION_CALLBACK = FLAG(9),
    SPELL_PROP_AREA_OF_EFFECT             = FLAG(10),
    SPELL_PROP_APPLIES_STATUS_EFFECT      = FLAG(11),
    SPELL_PROP_LIGHT_EMITTER              = FLAG(12),
} SpellProperties;

typedef struct {
    SpellProperties properties;
    f32 cast_duration;

    /* Flag specific fields */
    Sprite sprite;

    struct {
	// TODO: pack base_damage and penetration_values into struct
	DamageRange base_damage;
	Damage penetration_values;
	RetriggerBehaviour retrigger_behaviour;
    } damaging;

    struct {
	f32 projectile_speed;
	Vector2 collider_size;
	s32 extra_projectile_count;
	f32 projectile_cone_in_radians;
    } projectile;

    struct {
	f32 base_radius;
    } aoe;

    f32 lifetime;

    ParticleSpawnerConfig particle_spawner;
    ParticleSpawnerConfig on_death_particle_spawner;

    CallbackFunction hostile_collision_callback;

    struct {
	StatusEffectID effect;
	RetriggerBehaviour retrigger_behaviour;
    } applies_status_effects;

    LightSource light_emitter;

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

void magic_initialize(SpellArray *spells);
void magic_cast_spell(struct World *world, SpellID id, struct Entity *caster, Vector2 target_pos);
void magic_set_global_spell_array(SpellArray *spells);
void magic_add_to_spellbook(struct SpellCasterComponent *spellcaster, SpellID id);
String spell_type_to_string(SpellID id);
SpellID get_spell_at_spellbook_index(struct SpellCasterComponent *spellcaster, ssize index);
f32 get_spell_cast_duration(SpellID id);

#endif //MAGIC_H
