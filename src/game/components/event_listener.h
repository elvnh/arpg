#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include "base/vector.h"
#include "entity/entity_arena.h"
#include "entity/entity_id.h"
#include "magic.h"
#include "event_callback.h"

#define MAX_CALLBACKS_PER_EVENT_TYPE 2

struct LinearArena;
struct World;
struct Entity;

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

#endif //EVENT_LISTENER_H
