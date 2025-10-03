#ifndef HASH_H
#define HASH_H

#include "base/string8.h"

static inline u64 hash_string(String string)
{
    u64 result = 0xcbf29ce484222325;

    for (ssize i = 0; i < string.length; ++i) {
        result = result ^ (u64)string.data[i];
        result = result * 0x100000001b3;
    }

    return result;
}

#endif //HASH_H
