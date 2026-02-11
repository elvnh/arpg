#ifndef NAME_H
#define NAME_H

#include "base/string8.h"
#include "base/utils.h"

typedef struct {
    char data[64];
    s32  length;
} NameComponent;

static inline NameComponent name_component(String name)
{
    NameComponent result = {0};
    ASSERT(name.length <= ARRAY_COUNT(result.data));

    memcpy(result.data, name.data, ssize_to_usize(name.length));
    result.length = (s32)name.length;

    return result;
}

#endif //NAME_H
