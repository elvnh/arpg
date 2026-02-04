#ifndef DIRECTION_H
#define DIRECTION_H

#include "vector.h"

typedef enum {
    CARDINAL_DIR_WEST,
    CARDINAL_DIR_NORTH,
    CARDINAL_DIR_EAST,
    CARDINAL_DIR_SOUTH,
    CARDINAL_DIR_COUNT,
} CardinalDirection;

static inline Vector2 cardinal_direction_vector(CardinalDirection dir)
{
    switch (dir) {
        case CARDINAL_DIR_NORTH: return (Vector2)  { 0,   1};
        case CARDINAL_DIR_EAST:  return (Vector2)  { 1,   0};
        case CARDINAL_DIR_SOUTH: return (Vector2)  { 0,  -1};
        case CARDINAL_DIR_WEST:  return (Vector2)  {-1,   0};
        case CARDINAL_DIR_COUNT: break;
    }

    ASSERT(0);
    return (Vector2){0};
}

#endif //DIRECTION_H
