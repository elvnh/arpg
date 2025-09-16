#ifndef CAMERA_H
#define CAMERA_H

#include "base/typedefs.h"

#define ZOOM_MIN_VALUE  0.0f
#define ZOOM_MAX_VALUE  100.0f
#define ZOOM_SPEED      0.03f

// TODO: lerping to target position

typedef struct {
    Vector2 position;
    f32 zoom;
} Camera;

static inline Matrix4 camera_get_matrix(Camera cam, s32 window_width, s32 window_height)
{
    Matrix4 result = mat4_orthographic(window_width, window_height, Y_IS_UP);

    Vector2 window_dims = {(f32)window_width / 2.0f, (f32)window_height / 2.0f};
    result = mat4_translate(result, v2_add(window_dims, v2_neg(cam.position)));

    f32 scale = 1.0f + cam.zoom;

    result = mat4_scale(result, scale);

    return result;
}

static inline void camera_change_zoom(Camera *cam, f32 delta)
{
    f32 new_zoom = cam->zoom + delta;

    cam->zoom = CLAMP(new_zoom, ZOOM_MIN_VALUE, ZOOM_MAX_VALUE);
}

static inline void camera_zoom(Camera *cam, s32 direction)
{
    if (direction != 0) {
        ASSERT((direction == -1) || (direction == 1));
        f32 x = (direction == -1) ? (1.0f - ZOOM_SPEED) : (1.0f + ZOOM_SPEED);
        f32 new_zoom = (1.0f + cam->zoom) * x;

        cam->zoom = CLAMP(new_zoom - 1.0f, ZOOM_MIN_VALUE, ZOOM_MAX_VALUE);
    }
}

#endif //CAMERA_H
