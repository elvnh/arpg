#ifndef CAMERA_H
#define CAMERA_H

#include "base/maths.h"
#include "base/matrix.h"

#define ZOOM_MIN_VALUE          -0.9f
#define ZOOM_MAX_VALUE           10.0f
#define ZOOM_SPEED               0.15f
#define ZOOM_INTERPOLATE_SPEED   7.0f
#define CAMERA_FOLLOW_SPEED      10.0f

typedef struct {
    Vector2 position;
    Vector2 target_position;
    f32 zoom;
    f32 target_zoom;
} Camera;

static inline Matrix4 camera_get_matrix(Camera cam, Vector2i window_dims)
{
    Matrix4 result = mat4_orthographic(window_dims, Y_IS_UP);

    Vector2 window_center = {(f32)window_dims.x / 2.0f, (f32)window_dims.y / 2.0f};
    result = mat4_translate(result, v2_add(window_center, v2_neg(cam.position)));

    f32 scale = 1.0f + cam.zoom;

    result = mat4_scale(result, scale);

    return result;
}

static inline void camera_update(Camera *cam, f32 dt)
{
    f32 new_zoom = interpolate_sin(cam->zoom, cam->target_zoom, MIN(dt * ZOOM_INTERPOLATE_SPEED, 1.0f));
    cam->zoom = CLAMP(new_zoom, ZOOM_MIN_VALUE, ZOOM_MAX_VALUE);

    cam->position = v2_interpolate(cam->position, cam->target_position, MIN(dt * CAMERA_FOLLOW_SPEED, 1.0f));
}

static inline void camera_set_target(Camera *cam, Vector2 target)
{
    cam->target_position = target;
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

static inline Vector2 screen_to_world_coords(Camera cam, Vector2 v, Vector2i window_size)
{
    Vector2 result = {0};

    f32 scale = 1.0f + cam.zoom;

    f32 center_x = ((f32)window_size.x / 2.0f);
    f32 center_y = ((f32)window_size.y / 2.0f);
    result.x = (v.x - center_x) / scale + cam.position.x;
    result.y = (center_y - v.y) / scale + cam.position.y;

    return result;
}

static inline Vector2 screen_to_world_vector(Camera cam, Vector2 v)
{
    Vector2 result = {0};

    f32 scale = 1.0f + cam.zoom;

    result.x = v.x / scale;
    result.y = v.y / scale;

    return result;
}

static inline Rectangle camera_get_visible_area(Camera cam, Vector2i window_size)
{
    Vector2 dims = screen_to_world_vector(cam, v2i_to_v2(window_size));

    f32 min_x = cam.position.x - dims.x / 2.0f;
    f32 min_y = cam.position.y - dims.y / 2.0f;

    Rectangle result = {v2(min_x, min_y), dims};

    return result;
}

#endif //CAMERA_H
