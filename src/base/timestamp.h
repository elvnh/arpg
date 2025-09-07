#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include "typedefs.h"

typedef struct {
    s64 seconds;
    s64 nanoseconds;
} Timestamp;

static inline b32 timestamp_less_than(Timestamp lhs, Timestamp rhs)
{
    if (lhs.seconds > rhs.seconds) {
        return false;
    }

    return (lhs.seconds < rhs.seconds) || (lhs.nanoseconds < rhs.nanoseconds);
}

#endif //TIMESTAMP_H
