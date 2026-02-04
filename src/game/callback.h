#ifndef CALLBACK_H
#define CALLBACK_H

#include "base/typedefs.h"
#include "collision_policy.h"
#include "components/collider.h"

struct LinearArena;
struct World;
struct Entity;

typedef enum {
    EVENT_HOSTILE_COLLISION,
    EVENT_TILE_COLLISION,
    EVENT_ENTITY_DIED,
    EVENT_COUNT,
} EventType;

typedef struct {
    EventType event_type;

    struct Entity *self;
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

typedef void (*CallbackFunction)(void *, EventData);

typedef struct Callback {
    void *user_data;
    CallbackFunction function;
    struct Callback *next;
} Callback;

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


#endif //CALLBACK_H
