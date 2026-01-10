#ifndef CALLBACK_H
#define CALLBACK_H

#include "base/typedefs.h"
#include "collision_policy.h"
#include "components/collider.h"

struct LinearArena;
struct World;
struct Entity;

typedef enum {
    EVENT_COLLISION,
    EVENT_ENTITY_DIED,
    EVENT_COUNT,
} EventType;

typedef struct {
    EventType event_type;

    struct Entity *self;
    struct World *world;

    union {
	struct {
	    ObjectKind colliding_with_type;

	    union {
		struct Entity *entity;
		Vector2i tilemap_coords;
	    } collidee_as;
	} collision;
    } as;
} EventData;

typedef void (*CallbackFunction)(void *, EventData);

typedef struct Callback {
    void *user_data;
    CallbackFunction function;
    struct Callback *next;
} Callback;

static inline EventData event_data_death()
{
    EventData result = {0};
    result.event_type = EVENT_ENTITY_DIED;

    return result;
}

static inline EventData event_data_entity_collision(struct Entity *entity)
{
    EventData result = {0};
    result.event_type = EVENT_COLLISION;

    result.as.collision.colliding_with_type = OBJECT_KIND_ENTITIES;
    result.as.collision.collidee_as.entity = entity;

    return result;
}

static inline EventData event_data_tilemap_collision(Vector2i tilemap_coords)
{
    EventData result = {0};
    result.event_type = EVENT_COLLISION;

    result.as.collision.colliding_with_type = OBJECT_KIND_TILES;
    result.as.collision.collidee_as.tilemap_coords = tilemap_coords;

    return result;
}


#endif //CALLBACK_H
