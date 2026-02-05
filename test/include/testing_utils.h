#ifndef TESTING_UTILS_H
#define TESTING_UTILS_H

#include "base/typedefs.h"

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

#endif //TESTING_UTILS_H
