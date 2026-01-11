#include "magic.h"
#include "base/maths.h"
#include "base/utils.h"
#include "callback.h"
#include "collision_policy.h"
#include "components/event_listener.h"
#include "components/collider.h"
#include "components/particle.h"
#include "game/collision.h"
#include "game/damage.h"
#include "trigger.h"
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
	| SPELL_PROP_SPRITE | SPELL_PROP_DIE_ON_WALL_COLLISION | SPELL_PROP_DIE_ON_ENTITY_COLLISION
	/*| SPELL_PROP_PARTICLE_SPAWNER*/;

    spell.sprite.texture = get_asset_table()->fireball_texture;
    spell.sprite.size = v2(32, 32);
    spell.sprite.rotation_behaviour = SPRITE_ROTATE_BASED_ON_DIR;

    spell.projectile.projectile_speed = 300.0f;
    spell.projectile.collider_size = v2(32, 32);

    DamageRange damage_range = {0};
    set_damage_value_for_type(&damage_range.low_roll, DMG_TYPE_FIRE, 10);
    set_damage_value_for_type(&damage_range.high_roll, DMG_TYPE_FIRE, 20);

    spell.damaging.base_damage = damage_range;
    spell.damaging.retrigger_behaviour = RETRIGGER_NEVER;

/*
    spell.particle_spawner = (ParticleSpawnerConfig) {
	.kind = PS_SPAWN_DISTRIBUTED,
	.particle_color = {1, 0.4f, 0, 0.08f},
	.particle_size = 4.0f,
	.particle_lifetime = 1.0f,
	.particle_speed = 30.0f,
	.infinite = true,
	.particles_per_second = 100
    };
*/
    return spell;
}


static Spell spell_spark()
{
    Spell spell = {0};

    // TODO: erratic movement

    spell.properties = SPELL_PROP_PROJECTILE | SPELL_PROP_SPRITE | SPELL_PROP_DAMAGING
	| SPELL_PROP_BOUNCE_ON_TILES | SPELL_PROP_LIFETIME/* | SPELL_PROP_PARTICLE_SPAWNER*/;

    spell.sprite.texture = get_asset_table()->spark_texture;
    spell.sprite.size = v2(32, 32);

    spell.projectile.projectile_speed = 500.0f;
    spell.projectile.collider_size = v2(32, 32);
    spell.projectile.extra_projectile_count = 5;
    spell.projectile.projectile_cone_in_radians = deg_to_rad(90);

    spell.lifetime = 5.0f;

    set_damage_range_for_type(&spell.damaging.base_damage, DMG_TYPE_LIGHTNING, 1, 100);
    set_damage_value_for_type(&spell.damaging.penetration_values, DMG_TYPE_LIGHTNING, 20);
    spell.damaging.retrigger_behaviour = RETRIGGER_AFTER_NON_CONTACT;

/*
    spell.particle_spawner = (ParticleSpawnerConfig) {
	.kind = PS_SPAWN_DISTRIBUTED,
	.particle_color = {1.0f, 1.0f, 0, 0.15f},
	.particle_size = 3.0f,
	.particle_lifetime = 0.25f,
	.particle_speed = 150.0f,
	.infinite = true,
	.particles_per_second = 40
    };
*/

    return spell;
}

typedef struct {
    EntityID caster_id;
} SpellCallbackData;

static void spell_unset_property(Spell *spell, SpellProperties prop)
{
    ASSERT(is_pow2(prop));

    spell->properties &= ~prop;
}

static Spell spell_ice_shard();

static Spell spell_ice_shard_trigger()
{
    Spell spell = spell_ice_shard();

    spell_unset_property(&spell, SPELL_PROP_COLLISION_CALLBACK);

    spell.collision_callback = 0;
    spell.projectile.extra_projectile_count = 8;
    spell.projectile.projectile_cone_in_radians = deg_to_rad(180);

    return spell;
}

static void ice_shard_collision_callback(void *user_data, EventData event_data)
{
    SpellCallbackData *cb_data = (SpellCallbackData *)user_data;

    // TODO: separate event types for tiles and entities collisions
    if (event_data.as.collision.colliding_with_type == OBJECT_KIND_TILES) {
	return;
    }

    // TODO: make it so that the projectiles don't get immediately consumed since they spawn
    // inside enemy body

    Entity *caster = es_get_entity(event_data.world->entity_system, cb_data->caster_id);

    // TODO: we know which entity we just collided with, pass it along to magic_cast_spell
    // and have that function add a trigger cooldown between that entity and these spells

    magic_cast_spell(event_data.world, SPELL_ICE_SHARD_TRIGGER, caster,
	event_data.self->position, event_data.self->direction);
}

static Spell spell_ice_shard()
{
    Spell spell = {0};

    spell.properties = SPELL_PROP_PROJECTILE | SPELL_PROP_PROJECTILE | SPELL_PROP_DAMAGING
	| SPELL_PROP_SPRITE | SPELL_PROP_DIE_ON_WALL_COLLISION | SPELL_PROP_DIE_ON_ENTITY_COLLISION
	| SPELL_PROP_COLLISION_CALLBACK;

    spell.sprite.texture = get_asset_table()->ice_shard_texture;
    spell.sprite.size = v2(16, 16);
    spell.sprite.rotation_behaviour = SPRITE_ROTATE_BASED_ON_DIR;

    spell.projectile.projectile_speed = 300.0f;
    spell.projectile.collider_size = v2(32, 32);

    DamageRange damage_range = {0};
    set_damage_value_for_type(&damage_range.low_roll, DMG_TYPE_FIRE, 50);
    set_damage_value_for_type(&damage_range.high_roll, DMG_TYPE_FIRE, 70);

    spell.damaging.base_damage = damage_range;
    spell.damaging.retrigger_behaviour = RETRIGGER_NEVER;

    spell.collision_callback = ice_shard_collision_callback;

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
    spells->spells[SPELL_ICE_SHARD] = spell_ice_shard();
    spells->spells[SPELL_ICE_SHARD_TRIGGER] = spell_ice_shard_trigger();

    magic_set_global_spell_array(spells);
}

static const Spell *get_spell_by_id(SpellID id)
{
    ASSERT(id >= 0);
    ASSERT(id < SPELL_COUNT);

    return &g_spells->spells[id];
}

typedef struct {
    int i;
} UserData;

static void collision_function(void *user_data, EventData event_data)
{
    (void)user_data;
    (void)event_data;
    /* UserData *data = user_data; */

    printf("Colliding with: ");

    if (event_data.as.collision.colliding_with_type == OBJECT_KIND_ENTITIES) {
	printf("entity");
    } else {
	printf("wall");
    }

    printf("\n");

    /* DEBUG_BREAK; */
}

static void spawn_particles_on_death(void *user_data, EventData event_data)
{
    //DEBUG_BREAK;

    ParticleSpawnerConfig *particle_config = user_data;

    EntityWithID ps_entity = world_spawn_entity(event_data.world, FACTION_NEUTRAL);
    ps_entity.entity->position = event_data.self->position;

    ParticleSpawner *ps = es_add_component(ps_entity.entity, ParticleSpawner);
    particle_spawner_initialize(ps, *particle_config);
    ps->action_when_done = PS_WHEN_DONE_REMOVE_ENTITY;
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
	set_collision_policy_vs_tilemaps(spell_collider, COLLISION_POLICY_BOUNCE);
    }

    if (spell_has_prop(spell, SPELL_PROP_DAMAGING)) {
	DamageValues damage_roll = roll_damage_in_range(spell->damaging.base_damage);
        DamageValues damage_after_boosts = calculate_damage_dealt(damage_roll, caster,
	    world->item_system);

	DamageFieldComponent *dmg_field = es_add_component(spell_entity, DamageFieldComponent);
        /* // TODO: create_damage_instance function */
        DamageInstance damage = { damage_after_boosts, spell->damaging.penetration_values };
	dmg_field->damage = damage;
	dmg_field->retrigger_behaviour = spell->damaging.retrigger_behaviour;
    }

    if (spell_has_prop(spell, SPELL_PROP_DIE_ON_ENTITY_COLLISION)) {
	set_collision_policy_vs_hostile_faction(spell_collider, COLLISION_POLICY_DIE, caster->faction);
    }

    if (spell_has_prop(spell, SPELL_PROP_DIE_ON_WALL_COLLISION)) {
	set_collision_policy_vs_tilemaps(spell_collider, COLLISION_POLICY_DIE);
    }

    if (spell_has_prop(spell, SPELL_PROP_LIFETIME)) {
	ASSERT(spell->lifetime > 0.0f);

	LifetimeComponent *lifetime = es_add_component(spell_entity, LifetimeComponent);
	lifetime->time_to_live = spell->lifetime;
    }

    if (spell_has_prop(spell, SPELL_PROP_PARTICLE_SPAWNER)) {
	ParticleSpawner *ps = es_get_or_add_component(spell_entity, ParticleSpawner);
	particle_spawner_initialize(ps, spell->particle_spawner);
    }

    if (spell_has_prop(spell, SPELL_PROP_COLLISION_CALLBACK)) {
	es_get_or_add_component(spell_entity, EventListenerComponent);

	ASSERT(spell->collision_callback);

	SpellCallbackData data = {0};
	data.caster_id = es_get_id_of_entity(world->entity_system, caster);
	add_event_callback(spell_entity, EVENT_COLLISION, spell->collision_callback, &data);
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
	f32 cone = spell->projectile.projectile_cone_in_radians;

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

void magic_add_to_spellbook(SpellCasterComponent *spellcaster, SpellID id)
{
    ASSERT(spellcaster->spell_count < SPELL_COUNT);

#if 1 // TODO: only in debug builds
    for (ssize i = 0; i < spellcaster->spell_count; ++i) {
	if (spellcaster->spellbook[i] == id) {
	    ASSERT(0);
	}
    }
#endif

    ssize index = spellcaster->spell_count++;
    spellcaster->spellbook[index] = id;
}

String spell_type_to_string(SpellID id)
{
    switch (id) {
	case SPELL_FIREBALL: return str_lit("Fireball");
	case SPELL_SPARK: return str_lit("Spark");
	case SPELL_ICE_SHARD: return str_lit("Ice shard");
	case SPELL_ICE_SHARD_TRIGGER: return str_lit("Ice shard trigger");
	case SPELL_COUNT: ASSERT(0);
    }

    ASSERT(0);

    return (String){0};
}

SpellID get_spell_at_spellbook_index(SpellCasterComponent *spellcaster, ssize index)
{
    ASSERT(index < spellcaster->spell_count);
    ASSERT(index < SPELL_COUNT);

    SpellID result = spellcaster->spellbook[index];

    return result;
}
