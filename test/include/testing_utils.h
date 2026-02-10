#ifndef TESTING_UTILS_H
#define TESTING_UTILS_H

#include "base/typedefs.h"
#include "game/entity/entity_system.h"

static inline b32 is_zeroed(void *ptr, ssize size)
{
    b32 result = true;
    byte *bytes = ptr;

    for (ssize i = 0; i < size; ++i) {
        if (bytes[i] != 0) {
            result = false;
            break;
        }
    }

    return result;
}

// Entity systems need to be heap allocated since they're fairly large
static inline EntitySystem *allocate_entity_system(void)
{
    EntitySystem *entity_sys = calloc(1, sizeof(EntitySystem));
    es_initialize(entity_sys);

    return entity_sys;
}

static inline void free_entity_system(EntitySystem *es)
{
    free(es);
}

#endif //TESTING_UTILS_H
