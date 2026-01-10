#ifndef CALLBACK_H
#define CALLBACK_H

#include "base/typedefs.h"
#include "collision_policy.h"
#include "components/collider.h"

#define add_callback(entity, type, func, data) add_callback_impl(entity, type, func, data, \
	SIZEOF(*data), ALIGNOF(*data))

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

typedef struct {
    Callback *callbacks[EVENT_COUNT];
} CallbackComponent;

void send_event(struct Entity *entity, EventData event_data, struct World *world);
void add_callback_impl(struct Entity *entity, EventType event_type, CallbackFunction func,
    void *user_data, ssize data_size, ssize data_alignment);

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
