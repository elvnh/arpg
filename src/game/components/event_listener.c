#include "event_listener.h"
#include "entity/entity_arena.h"
#include "entity/entity_system.h"
#include "world/world.h"
#include "base/utils.h"

#include <string.h>

void add_event_callback_impl(Entity *entity, EventType event_type, CallbackFunction func,
    const void *user_data, s32 data_size, s32 data_alignment)
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
        EntityArenaAllocation user_data_copy =
            entity_arena_allocate(&entity->entity_arena, data_size, data_alignment);

	memcpy(user_data_copy.ptr, user_data, ssize_to_usize(data_size));

        cb->user_data_arena_index = user_data_copy.index;
    } else {
        cb->user_data_arena_index.offset = -1;
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

            void *user_data = entity_arena_get(&entity->entity_arena, current_cb->user_data_arena_index);
	    current_cb->function(user_data, event_data, frame_arena);
        }
    }
}
