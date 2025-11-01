#ifndef ENTITY_STATE_H
#define ENTITY_STATE_H

// NOTE: this needs to be in a separate file to avoid
// a circular dependency
typedef enum {
    ENTITY_STATE_IDLE,
    ENTITY_STATE_WALKING,
    ENTITY_STATE_COUNT,
} EntityState;


#endif //ENTITY_STATE_H
