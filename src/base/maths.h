#ifndef MATHS_H
#define MATHS_H

#include <math.h>

#include "typedefs.h"

static inline f32 abs_f32(f32 n)
{
    f32 result = (f32)fabs(n);
    return result;
}

#endif //MATHS_H
