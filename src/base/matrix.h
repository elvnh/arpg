#ifndef MATRIX_H
#define MATRIX_H

#include "base/typedefs.h"

typedef struct {
    f32 data[4][4];
} Matrix4;

inline static Matrix4 mat4_identity()
{
    Matrix4 m = {0};

    m.data[0][0] = 1.0f;
    m.data[1][1] = 1.0f;
    m.data[2][2] = 1.0f;
    m.data[3][3] = 1.0f;

    return m;
}

inline static Matrix4 mat4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far)
{
    Matrix4 m = {0};

    m.data[0][0] =  2.0f / (right - left);
    m.data[1][1] =  2.0f / (top - bottom);
    m.data[2][2] = -2.0f / (z_far - z_near);
    m.data[3][3] =  1.0f;

    m.data[3][0] = -((right + left) / (right - left));
    m.data[3][1] = -((top + bottom) / (top - bottom));
    m.data[3][2] = -((z_far + z_near) / (z_far - z_near));

    return m;
}

inline static f32 *mat4_ptr(const Matrix4 *mat)
{
    return (f32 *)mat->data;
}

#endif //MATRIX_H
