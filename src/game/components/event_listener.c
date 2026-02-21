#include "event_listener.h"
#include "entity/entity_arena.h"
#include "entity/entity_system.h"
#include "world/world.h"
#include "base/utils.h"

#include <string.h>

void add_event_callback(Entity *entity, EventType event_type, CallbackFunction func,
    CallbackUserData *user_data)
{
    ASSERT(event_type >= 0);
    ASSERT(event_type < EVENT_COUNT);

    EventListenerComponent *comp = es_get_component(entity, EventListenerComponent);
    ASSERT(comp);

    PerEventTypeCallbacks *cb_list = &comp->per_event_callbacks[event_type];
    ASSERT(cb_list->count < ARRAY_COUNT(cb_list->callbacks));

    EventCallback *cb = &cb_list->callbacks[cb_list->count++];
    cb->function = func;

    if (user_data) {
	cb->user_data = *user_data;
    }
}

void send_event_to_entity(Entity *entity, EventData event_data, World *world, LinearArena *frame_arena)
{
    ASSERT(event_data.event_type >= 0);
    ASSERT(event_data.event_type < EVENT_COUNT);

    EventListenerComponent *comp = es_get_component(entity, EventListenerComponent);

    if (comp) {
        event_data.receiver_id = es_get_id_of_entity(&world->entity_system, entity);
        event_data.world = world;

        PerEventTypeCallbacks *cb_list = &comp->per_event_callbacks[event_data.event_type];

        for (s32 i = 0; i < cb_list->count; ++i) {
            EventCallback *current_cb = &cb_list->callbacks[i];
	    ASSERT(current_cb->function);

	    current_cb->function(current_cb->user_data, event_data, frame_arena);
        }
    }
}
