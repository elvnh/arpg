#ifndef EVENT_CALLBACK_H
#define EVENT_CALLBACK_H

#include "components/particle_spawner.h"
#include "entity/entity_id.h"
#include "magic.h"

struct LinearArena;

/*
  TODO:
  - Make it easier to construct user data
 */

// NOTE: callbacks will be invalid after hot reloading!!!

typedef enum {
    EVENT_HOSTILE_COLLISION,
    EVENT_TILE_COLLISION,
    EVENT_ENTITY_DIED,
    EVENT_COUNT,
} EventType;

typedef struct {
    EventType event_type;

    EntityID receiver_id;
    struct World *world;

    union {
	struct {
	    EntityID collided_with;
	} hostile_collision;

	struct {
	    Vector2i tile_coords;
	} tile_collision;
    } as;
} EventData;

typedef struct {
    EntityID caster_id;

    union {
	struct {
	    s32 chains_remaining;
	    f32 search_area_size;
	} chain;

	struct {
	    s32 fork_count;
	    SpellID fork_spell;
	} fork;
    } as;
} SpellCallbackData;

typedef union {
    SpellCallbackData    spell_data;
    ParticleSpawnerSetup particle_spawner_data;
} CallbackUserData;

typedef void (*CallbackFunction)(CallbackUserData, EventData, struct LinearArena *);

typedef struct {
    CallbackFunction function;
    CallbackUserData user_data;
} EventCallback;

static inline EventData event_data_death(void)
{
    EventData result = {0};
    result.event_type = EVENT_ENTITY_DIED;

    return result;
}

static inline EventData event_data_hostile_collision(EntityID entity_id)
{
    EventData result = {0};
    result.event_type = EVENT_HOSTILE_COLLISION;

    result.as.hostile_collision.collided_with = entity_id;

    return result;
}

static inline EventData event_data_tilemap_collision(Vector2i tilemap_coords)
{
    EventData result = {0};
    result.event_type = EVENT_TILE_COLLISION;

    result.as.tile_collision.tile_coords = tilemap_coords;

    return result;
}

#endif //EVENT_CALLBACK_H
