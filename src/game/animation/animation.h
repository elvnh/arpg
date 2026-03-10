#ifndef ANIMATION_H
#define ANIMATION_H

#include "entity/entity_state.h"

typedef struct {
    enum {
        ANIM_ON_END_DO_NOTHING,
        ANIM_ON_END_REPEAT,
        ANIM_ON_END_TRANSITION_TO_STATE,
        ANIM_ON_END_REMOVE_COMPONENT,
    } kind;

    union {
        EntityState state_transition;
    } as;
} AnimationOnEnd;

#endif //ANIMATION_H
