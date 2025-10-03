#ifndef MATRIX_H
#define MATRIX_H

#include "base/typedefs.h"
#include "base/vector.h"

// TODO: move elsewhere
typedef enum {
    Y_IS_UP,
    Y_IS_DOWN
} YDirection;

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

inline static Matrix4 mat4_orthographic_base(f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far)
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

inline static Matrix4 mat4_orthographic(Vector2i window_size, YDirection y_direction)
{
    Matrix4 result = {0};

    if (y_direction == Y_IS_UP) {
        result = mat4_orthographic_base(0.0f, (f32)window_size.x, 0.0f, (f32)window_size.y, 0.1f, 100.0f);
    } else {
        result = mat4_orthographic_base(0.0f, (f32)window_size.x, (f32)window_size.y, 0.0f, 0.1f, 100.0f);
    }

    return result;
}

inline static Matrix4 mat4_translate(Matrix4 m, Vector2 v)
{
    m.data[3][0] += (m.data[0][0] * v.x) + (m.data[1][0] * v.y);
    m.data[3][1] += (m.data[0][1] * v.x) + (m.data[1][1] * v.y);
    m.data[3][2] += (m.data[0][2] * v.x) + (m.data[1][2] * v.y);
    m.data[3][3] += (m.data[0][3] * v.x) + (m.data[1][3] * v.y);

    return m;
}

inline static Matrix4 mat4_scale(Matrix4 m, f32 scalar)
{
    for (s32 col = 0; col < 4; ++col) {
        for (s32 row = 0; row < 4; ++row) {
            m.data[col][row] *= scalar;
        }
    }

    return m;
}

inline static f32 *mat4_ptr(const Matrix4 *mat)
{
    return (f32 *)mat->data;
}

#endif //MATRIX_H
