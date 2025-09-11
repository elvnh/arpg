#ifndef RGBA_H
#define RGBA_H

#include "typedefs.h"

#define RGBA32_WHITE (RGBA32) {1.0f, 1.0f, 1.0f, 1.0f}
#define RGBA32_BLUE (RGBA32)  {0.0f, 0.0f, 1.0f, 1.0f}
#define RGBA32_RED (RGBA32)   {1.0f, 0.0f, 0.0f, 1.0f}

typedef struct {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
} RGBA32;


#endif //RGBA_H
