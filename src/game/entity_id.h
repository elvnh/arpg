#ifndef ENTITY_ID_H
#define ENTITY_ID_H

#include "base/typedefs.h"

typedef s32 EntityIndex;
typedef s32 EntityGeneration;

typedef struct {
    EntityIndex      slot_index;
    EntityGeneration generation;
} EntityID;

#endif //ENTITY_ID_H
