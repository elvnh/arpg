#include "magic.h"
#include "base/maths.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "collision/collision_policy.h"
#include "components/event_listener.h"
#include "collision/collider.h"
#include "components/particle.h"
#include "light.h"
#include "status_effect.h"
#include "collision/collision.h"
#include "game/damage.h"
#include "stats.h"
#include "collision/trigger.h"
#include "world/world.h"
#include "components/component.h"
#include "platform/asset.h"
#include "entity/entity_system.h"
#include "asset_table.h"

typedef struct SpellArray {
    Spell spells[SPELL_COUNT];
} SpellArray;

typedef struct {
    EntityID caster_id;
} SpellCallbackData;

static SpellArray g_spells = {0};

// TODO: move to callback file
static void spawn_particles_on_death(void *user_data, EventData event_data)
{
    ParticleSpawnerConfig *particle_config = user_data;

    EntityWithID ps_entity = world_spawn_entity(event_data.world, FACTION_NEUTRAL);
    ps_entity.entity->position = event_data.self->position;

    ParticleSpawner *ps = es_add_component(ps_entity.entity, ParticleSpawner);
    particle_spawner_initialize(ps, *particle_config);
    ps->action_when_done = PS_WHEN_DONE_REMOVE_ENTITY;
}

static const Spell *get_spell_by_id(SpellID id)
{
    ASSERT(id >= 0);
    ASSERT(id < SPELL_COUNT);

    return &g_spells.spells[id];
}

static b32 spell_has_prop(const Spell *spell, SpellProperties prop)
{
    b32 result = (spell->properties & prop) != 0;

    return result;
}

static void spell_unset_property(Spell *spell, SpellProperties prop)
{
    ASSERT(is_pow2(prop));

    spell->properties &= ~prop;
}

static void cast_single_spell(World *world, const Spell *spell, Entity *caster,
    Vector2 spell_origin, Vector2 target_pos, Vector2 dir, Entity *cooldown_target,
    RetriggerBehaviour cooldown_retrigger)
{
    EntityWithID spell_entity_with_id = world_spawn_entity(world, caster->faction);
    Entity *spell_entity = spell_entity_with_id.entity;

    ColliderComponent *spell_collider = es_get_or_add_component(spell_entity, ColliderComponent);
    spell_collider->collision_group = COLLISION_GROUP_PROJECTILES;

    if (spell_has_prop(spell, SPELL_PROP_SPRITE)) {
	SpriteComponent *sprite_comp = es_get_or_add_component(spell_entity, SpriteComponent);

	sprite_comp->sprite = spell->sprite;
    }

    if (spell_has_prop(spell, SPELL_PROP_PROJECTILE)) {
	ASSERT(!spell_has_prop(spell, SPELL_PROP_AREA_OF_EFFECT));
	spell_collider->size = spell->projectile.collider_size;

	spell_entity->velocity = v2_mul_s(dir, spell->projectile.projectile_speed);
    }

    if (spell_has_prop(spell, SPELL_PROP_AREA_OF_EFFECT)) {
	ASSERT(!spell_has_prop(spell, SPELL_PROP_PROJECTILE));
	spell_collider->size = v2(spell->aoe.base_radius, spell->aoe.base_radius);

	SpriteComponent *sprite_comp = es_get_component(spell_entity, SpriteComponent);

	if (sprite_comp) {
	    sprite_comp->sprite.size = spell_collider->size;
	}
    }

    if (spell_has_prop(spell, SPELL_PROP_BOUNCE_ON_TILES)) {
	set_collision_policy_vs_tilemaps(spell_collider, COLLISION_POLICY_BOUNCE);
    }

    if (spell_has_prop(spell, SPELL_PROP_DAMAGING)) {
	Damage damage_roll = roll_damage_in_range(spell->damaging.base_damage);
        Damage damage_after_boosts = calculate_damage_dealt(damage_roll, caster,
	    &world->item_system);

	DamageFieldComponent *dmg_field = es_add_component(spell_entity, DamageFieldComponent);

        DamageInstance damage = {damage_after_boosts, spell->damaging.penetration_values};
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

    if (spell_has_prop(spell, SPELL_PROP_SPAWN_PARTICLES_ON_DEATH)) {
	ASSERT(!spell->on_death_particle_spawner.infinite);
	ASSERT(spell->on_death_particle_spawner.total_particles_to_spawn > 0);
	ASSERT(spell->on_death_particle_spawner.particle_size > 0.0f);
	ASSERT(spell->on_death_particle_spawner.particle_speed > 0.0f);
	ASSERT(spell->on_death_particle_spawner.particles_per_second > 0);

	es_get_or_add_component(spell_entity, EventListenerComponent);

	add_event_callback(spell_entity, EVENT_ENTITY_DIED, spawn_particles_on_death,
	    &spell->on_death_particle_spawner);
    }

    if (spell_has_prop(spell, SPELL_PROP_HOSTILE_COLLISION_CALLBACK)) {
	es_get_or_add_component(spell_entity, EventListenerComponent);

	ASSERT(spell->hostile_collision_callback);

	SpellCallbackData data = {0};
	data.caster_id = es_get_id_of_entity(&world->entity_system, caster);
	add_event_callback(spell_entity, EVENT_HOSTILE_COLLISION, spell->hostile_collision_callback, &data);
    }

    if (spell_has_prop(spell, SPELL_PROP_APPLIES_STATUS_EFFECT)) {
	EffectApplierComponent *ea = es_get_or_add_component(spell_entity, EffectApplierComponent);
	ea->effect = spell->applies_status_effects.effect;
	ea->retrigger_behaviour = spell->applies_status_effects.retrigger_behaviour;
    }

    if (spell_has_prop(spell, SPELL_PROP_LIGHT_EMITTER)) {
        LightEmitter *le = es_get_or_add_component(spell_entity, LightEmitter);
        le->light = spell->light_emitter;
    }

    if (spell_has_prop(spell, SPELL_PROP_PROJECTILE)) {
	spell_entity->position = spell_origin;
    } else {
	ASSERT(spell_has_prop(spell, SPELL_PROP_AREA_OF_EFFECT));
	spell_entity->position = target_pos;
    }

    // Offset so that spells center is centered on the target position
    Rectangle bounds = world_get_entity_bounding_box(spell_entity);
    spell_entity->position = v2_sub(spell_entity->position, v2_div_s(bounds.size, 2.0f));

    // Spells may need to be spawned with a collision cooldown against a specific entity,
    // likely the entity just collided with for forking spells so that the child spells
    // don't immediately collide with that entity
    if (cooldown_target) {
	EntityID spell_entity_id = spell_entity_with_id.id;
	EntityID target_id = es_get_id_of_entity(&world->entity_system, cooldown_target);

	// TODO: allow customizing which components are on cooldown, and setting multiple at once
	// For now, only add the collider so that nothing else can be
	// triggered until it's off cooldown
	world_add_trigger_cooldown(world, spell_entity_id, target_id,
	    component_flag(ColliderComponent), cooldown_retrigger);
    }
}

static void cast_spell_impl(struct World *world, SpellID id, struct Entity *caster,
    Vector2 spell_origin, Vector2 target_pos, Vector2 dir, Entity *cooldown_target,
    RetriggerBehaviour cooldown_retrigger)
{
    const Spell *spell = get_spell_by_id(id);
    s32 spell_count = 1 + spell->projectile.extra_projectile_count;

    f32 current_angle = atan2f(dir.y, dir.x);
    f32 angle_step_size = 0.0f;

    if (spell_count >= 2) {
	f32 cone = spell->projectile.projectile_cone_in_radians;

	current_angle -= cone / 2.0f;
	angle_step_size = cone / ((f32)(spell_count - 1));
    }

    for (s32 i = 0; i < spell_count; ++i) {
	Vector2 current_dir = v2(cos_f32(current_angle), sin_f32(current_angle));

	cast_single_spell(world, spell, caster, spell_origin, target_pos, current_dir,
	    cooldown_target, cooldown_retrigger);

	current_angle += angle_step_size;
    }
}

void magic_cast_spell(World *world, SpellID id, Entity *caster, Vector2 target_pos)
{
    Vector2 spell_origin = rect_center(world_get_entity_bounding_box(caster));
    Vector2 dir = v2_sub(target_pos, spell_origin);

    cast_spell_impl(world, id, caster, spell_origin, target_pos,
	dir, 0, retrigger_whenever());
}

static Spell spell_fireball(void)
{
    Spell spell = {0};

    spell.properties = SPELL_PROP_PROJECTILE | SPELL_PROP_PROJECTILE | SPELL_PROP_DAMAGING
	| SPELL_PROP_SPRITE | SPELL_PROP_DIE_ON_WALL_COLLISION | SPELL_PROP_DIE_ON_ENTITY_COLLISION
	| SPELL_PROP_PARTICLE_SPAWNER | SPELL_PROP_LIGHT_EMITTER;
    spell.cast_duration = 0.3f;

    spell.sprite = sprite_create(
	get_asset_table()->fireball_texture,
	v2(32, 32),
	SPRITE_ROTATE_BASED_ON_DIR
    );

    spell.projectile.projectile_speed = 300.0f;
    spell.projectile.collider_size = v2(32, 32);

    DamageRange damage_range = {0};
    set_damage_value(&damage_range.low_roll, DMG_TYPE_Fire, 10);
    set_damage_value(&damage_range.high_roll, DMG_TYPE_Fire, 20);

    spell.damaging.base_damage = damage_range;
    spell.damaging.retrigger_behaviour = retrigger_never();

    spell.light_emitter.radius = 250.0f;
    spell.light_emitter.color = RGBA32_RED;
    spell.light_emitter.kind = LIGHT_RAYCASTED;
    spell.light_emitter.fade_duration = 1.0f;

    spell.particle_spawner = (ParticleSpawnerConfig) {
	.kind = PS_SPAWN_DISTRIBUTED,
	.particle_color = {1, 0.4f, 0, 0.08f},
	.particle_size = 4.0f,
	.particle_lifetime = 2.0f,
	.particle_speed = 30.0f,
	.infinite = true,
	.particles_per_second = 100,
        .emits_light = true,
        .light_source = (LightSource){LIGHT_REGULAR, 2.5f, RGBA32_RED, false, 0, 0}
    };

    return spell;
}


static Spell spell_spark(void)
{
    Spell spell = {0};

    // TODO: erratic movement

    spell.properties = SPELL_PROP_PROJECTILE | SPELL_PROP_SPRITE | SPELL_PROP_DAMAGING
	| SPELL_PROP_BOUNCE_ON_TILES | SPELL_PROP_LIFETIME | SPELL_PROP_PARTICLE_SPAWNER;
    spell.cast_duration = 0.3f;

    spell.sprite = sprite_create(
	get_asset_table()->spark_texture,
	v2(32, 32),
	SPRITE_ROTATE_NONE
    );

    spell.projectile.projectile_speed = 500.0f;
    spell.projectile.collider_size = v2(32, 32);
    spell.projectile.extra_projectile_count = 5;
    spell.projectile.projectile_cone_in_radians = deg_to_rad(90);

    spell.lifetime = 5.0f;

    set_damage_range_for_type(&spell.damaging.base_damage, DMG_TYPE_Lightning, 1, 100);
    set_damage_value(&spell.damaging.penetration_values, DMG_TYPE_Lightning, 20);
    spell.damaging.retrigger_behaviour = retrigger_after_non_contact();

    spell.particle_spawner = (ParticleSpawnerConfig) {
	.kind = PS_SPAWN_DISTRIBUTED,
	.particle_color = {1.0f, 1.0f, 0, 0.15f},
	.particle_size = 3.0f,
	.particle_lifetime = 0.25f,
	.particle_speed = 150.0f,
	.infinite = true,
	.particles_per_second = 40
    };

    return spell;
}

static Spell spell_blizzard(void)
{
    Spell spell = {0};

    spell.properties = SPELL_PROP_AREA_OF_EFFECT | SPELL_PROP_SPRITE | SPELL_PROP_DAMAGING
	| SPELL_PROP_LIFETIME | SPELL_PROP_PARTICLE_SPAWNER | SPELL_PROP_APPLIES_STATUS_EFFECT;
    spell.cast_duration = 0.5f;

    spell.aoe.base_radius = 128.0f;

    spell.sprite = sprite_create_colored(
	get_asset_table()->blizzard_texture,
	V2_ZERO, // Sprite size is set equal to radius later on
	SPRITE_ROTATE_NONE,
	rgba32(1, 1, 1, 0.5f)
    );

    spell.lifetime = 15.0f;

    set_damage_range_for_type(&spell.damaging.base_damage, DMG_TYPE_Lightning, 1, 100);
    set_damage_value(&spell.damaging.penetration_values, DMG_TYPE_Lightning, 20);
    spell.damaging.retrigger_behaviour = retrigger_after_duration(1.0f);

    spell.particle_spawner = (ParticleSpawnerConfig) {
	.kind = PS_SPAWN_DISTRIBUTED,
	.particle_color = {0.15f, 0.5f, 1.0f, 0.25f},
	.particle_size = 3.0f,
	.particle_lifetime = 0.25f,
	.particle_speed = 150.0f,
	.infinite = true,
	.particles_per_second = 300
    };

    spell.applies_status_effects.effect = STATUS_EFFECT_CHILLED;
    spell.applies_status_effects.retrigger_behaviour = retrigger_whenever();

    return spell;
}

static void ice_shard_collision_callback(void *user_data, EventData event_data)
{
    SpellCallbackData *cb_data = (SpellCallbackData *)user_data;

    Entity *caster = es_get_entity(&event_data.world->entity_system, cb_data->caster_id);
    ASSERT(caster && "Entity died before impact collision callback was called. "
        "This should probably never happen?");

    Entity *collide_target = es_get_entity(&event_data.world->entity_system,
	event_data.as.hostile_collision.collided_with);

    // TODO: should this kind of behaviour be encoded in a forking property of spells?
    // NOTE: these have no target position, only origin and direction
    cast_spell_impl(event_data.world, SPELL_ICE_SHARD_TRIGGER, caster,
	event_data.self->position, V2_ZERO, event_data.self->direction,
	collide_target, retrigger_never());
}

static Spell spell_ice_shard(void)
{
    Spell spell = {0};

    spell.properties = SPELL_PROP_PROJECTILE | SPELL_PROP_PROJECTILE | SPELL_PROP_DAMAGING
	| SPELL_PROP_SPRITE | SPELL_PROP_DIE_ON_WALL_COLLISION | SPELL_PROP_DIE_ON_ENTITY_COLLISION
	| SPELL_PROP_HOSTILE_COLLISION_CALLBACK | SPELL_PROP_PARTICLE_SPAWNER
	| SPELL_PROP_SPAWN_PARTICLES_ON_DEATH;
    spell.cast_duration = 0.3f;

    spell.sprite = sprite_create(
	get_asset_table()->ice_shard_texture,
	v2(16, 16),
	SPRITE_ROTATE_BASED_ON_DIR
    );

    spell.projectile.projectile_speed = 300.0f;
    spell.projectile.collider_size = spell.sprite.size;

    DamageRange damage_range = {0};
    set_damage_value(&damage_range.low_roll, DMG_TYPE_Fire, 50);
    set_damage_value(&damage_range.high_roll, DMG_TYPE_Fire, 70);

    spell.damaging.base_damage = damage_range;
    spell.damaging.retrigger_behaviour = retrigger_never(); // TODO: unnecessary

    spell.hostile_collision_callback = ice_shard_collision_callback;

    spell.particle_spawner = (ParticleSpawnerConfig) {
	.kind = PS_SPAWN_DISTRIBUTED,
	.particle_color = {0.0f, 0.5f, 1.0f, 0.5f},
	.particle_size = 3.0f,
	.particle_lifetime = 0.5f,
	.particle_speed = 50.0f,
	.infinite = true,
	.particles_per_second = 40
    };

    spell.on_death_particle_spawner = spell.particle_spawner;
    spell.on_death_particle_spawner.kind = PS_SPAWN_ALL_AT_ONCE;
    spell.on_death_particle_spawner.infinite = false;
    spell.on_death_particle_spawner.total_particles_to_spawn = 10;

    return spell;
}

static Spell spell_ice_shard_trigger(void)
{
    Spell spell = spell_ice_shard();

    spell_unset_property(&spell, SPELL_PROP_HOSTILE_COLLISION_CALLBACK);
    spell.hostile_collision_callback = 0;

    spell.projectile.extra_projectile_count = 10;
    spell.projectile.projectile_cone_in_radians = deg_to_rad(360);

    spell.sprite.size = v2_div_s(spell.sprite.size, 2);
    spell.projectile.collider_size = spell.sprite.size;

    return spell;
}

void magic_initialize()
{
    g_spells.spells[SPELL_FIREBALL] = spell_fireball();
    g_spells.spells[SPELL_SPARK] = spell_spark();
    g_spells.spells[SPELL_ICE_SHARD] = spell_ice_shard();
    g_spells.spells[SPELL_ICE_SHARD_TRIGGER] = spell_ice_shard_trigger();
    g_spells.spells[SPELL_BLIZZARD] = spell_blizzard();
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
	case SPELL_BLIZZARD: return str_lit("Blizzard");
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

f32 get_spell_cast_duration(SpellID id)
{
    const Spell *spell = get_spell_by_id(id);

    f32 result = spell->cast_duration;

    return result;
}
