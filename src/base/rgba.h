#ifndef RGBA_H
#define RGBA_H

#include "typedefs.h"

#define RGBA32_BLACK rgba32(0.0f, 0.0f, 0.0f, 1.0f)
#define RGBA32_WHITE rgba32(1.0f, 1.0f, 1.0f, 1.0f)
#define RGBA32_RED rgba32(1.0f, 0.0f, 0.0f, 1.0f)
#define RGBA32_BLUE rgba32(0.0f, 0.0f, 1.0f, 1.0f)
#define RGBA32_GREEN rgba32(0.0f, 1.0f, 0.0f, 1.0f)
#define RGBA32_YELLOW rgba32(1.0f, 1.0f, 0.0f, 1.0f)
#define RGBA32_TRANSPARENT rgba32(0, 0, 0, 0)

typedef struct {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
} RGBA32;

static inline RGBA32 rgba32(f32 r, f32 g, f32 b, f32 a)
{
    return (RGBA32) {r, g, b, a};
}

#endif //RGBA_H
