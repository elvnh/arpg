#include "callback.h"
#include "entity_system.h"
#include "world.h"
#include "base/utils.h"

#include <string.h>

void add_callback_impl(struct Entity *entity, EventType event_type, CallbackFunction func,
    void *user_data, ssize data_size, ssize data_alignment)
{
    ASSERT(event_type >= 0);
    ASSERT(event_type < EVENT_COUNT);

    CallbackComponent *comp = es_get_component(entity, CallbackComponent);
    ASSERT(comp);

    Callback *cb = la_allocate_item(&entity->entity_arena, Callback);

    cb->user_data = la_allocate(&entity->entity_arena, 1, data_size, data_alignment);
    memcpy(cb->user_data, user_data, cast_ssize_to_usize(data_size));

    cb->function = func;

    cb->next = comp->callbacks[event_type];
    comp->callbacks[event_type] = cb;
}

void try_invoke_callback(Entity *entity, EventData event_data, World *world)
{
    ASSERT(event_data.event_type >= 0);
    ASSERT(event_data.event_type < EVENT_COUNT);

    CallbackComponent *comp = es_get_component(entity, CallbackComponent);
    event_data.self = entity;
    event_data.world = world;

    if (comp) {
	Callback *current_cb = comp->callbacks[event_data.event_type];

	while (current_cb) {
	    ASSERT(current_cb->function);

	    current_cb->function(current_cb->user_data, event_data);

	    current_cb = current_cb->next;
	}
    }
}
