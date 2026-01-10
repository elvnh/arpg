#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include "callback.h"

#define add_event_callback(entity, type, func, data) add_event_callback_impl(entity, type, func, data, \
	SIZEOF(*data), ALIGNOF(*data))

struct LinearArena;
struct World;
struct Entity;

typedef struct {
    Callback *callbacks[EVENT_COUNT];
} EventListenerComponent;

void send_event(struct Entity *entity, EventData event_data, struct World *world);
void add_event_callback_impl(struct Entity *entity, EventType event_type, CallbackFunction func,
    void *user_data, ssize data_size, ssize data_alignment);

#endif //EVENT_LISTENER_H
