#ifndef CALLBACK_FUNCTIONS_H
#define CALLBACK_FUNCTIONS_H

#include "event_callback.h"

// TODO: move more callbacks into this file

void callback_spawn_particles(CallbackUserData user_data, EventData event_data,
    struct LinearArena *frame_arena);

#endif //CALLBACK_FUNCTIONS_H
