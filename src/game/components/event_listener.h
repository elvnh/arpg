#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include "base/vector.h"
#include "components/particle_spawner.h"
#include "entity/entity_arena.h"
#include "entity/entity_id.h"
#include "magic.h"

#define MAX_CALLBACKS_PER_EVENT_TYPE 2

/*
  TODO:
  - Make it easier to construct user data
 */

struct LinearArena;
struct World;
struct Entity;

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

typedef union {
    SpellCallbackData    spell_callback;
    ParticleSpawnerSetup particle_spawner;
} CallbackUserData;

typedef void (*CallbackFunction)(CallbackUserData, EventData, struct LinearArena *);

typedef struct {
    CallbackFunction function;
    CallbackUserData user_data;
} EventCallback;

typedef struct {
    EventCallback callbacks[MAX_CALLBACKS_PER_EVENT_TYPE];
    s32 count;
} PerEventTypeCallbacks;

typedef struct {
    PerEventTypeCallbacks per_event_callbacks[EVENT_COUNT];
} EventListenerComponent;

void send_event_to_entity(struct Entity *entity, EventData event_data, struct World *world,
    struct LinearArena *frame_arena);
void add_event_callback(struct Entity *entity, EventType event_type, CallbackFunction func,
    CallbackUserData *user_data);

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

#endif //EVENT_LISTENER_H
