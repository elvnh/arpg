#ifndef RGBA_H
#define RGBA_H

#include "typedefs.h"

typedef struct {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
} RGBA32;

#define RGBA32_WHITE (RGBA32) {1.0f, 1.0f, 1.0f, 1.0f}

#endif //RGBA_H
