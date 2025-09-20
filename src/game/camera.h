#ifndef CAMERA_H
#define CAMERA_H

#include "base/maths.h"
#include "base/matrix.h"

#define ZOOM_MIN_VALUE          -0.9f
#define ZOOM_MAX_VALUE           10.0f
#define ZOOM_SPEED               0.15f
#define ZOOM_INTERPOLATE_SPEED   12.0f

typedef struct {
    Vector2 position;
    f32 zoom;
    f32 target_zoom;
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

static inline void camera_update(Camera *cam, f32 dt)
{
    f32 x = (1.0f + cam->zoom) / (1.0f + cam->target_zoom);
    f32 new_zoom = interpolate(cam->zoom, cam->target_zoom, x * ZOOM_INTERPOLATE_SPEED * dt);

    cam->zoom = CLAMP(new_zoom, ZOOM_MIN_VALUE, ZOOM_MAX_VALUE);
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
        f32 new_target = (1.0f + cam->target_zoom) * x;

        cam->target_zoom = CLAMP(new_target - 1.0f, ZOOM_MIN_VALUE, ZOOM_MAX_VALUE);
    }
}

static inline Vector2 screen_to_world_coords(Camera cam, Vector2 v,
    s32 window_width, s32 window_height)
{
    Vector2 result = {0};

    f32 scale = 1.0f + cam.zoom;

    f32 center_x = ((f32)window_width / 2.0f);
    f32 center_y = ((f32)window_height / 2.0f);
    result.x = (v.x - center_x) / scale + cam.position.x;
    result.y = (center_y - v.y) / scale + cam.position.y;

    return result;
}

#endif //CAMERA_H
