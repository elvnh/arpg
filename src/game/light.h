#ifndef LIGHT_H
#define LIGHT_H

#include "base/list.h"
#include "base/rectangle.h"
#include "base/polygon.h"
#include "base/rgba.h"

#define LIGHT_DEFAULT_FADE_OUT_TIME 0.15f

/*
  TODO:
  - Maybe return edge list as copy and then sort by closest to light source
    so that not as many checks are needed?
 */

struct Tilemap;
struct LinearArena;
struct RenderBatch;
struct World;

typedef enum {
    LIGHT_REGULAR,
    LIGHT_RAYCASTED,
} LightKind;

typedef struct {
    LightKind kind;
    f32 radius;
    RGBA32 color;

    b32 fading_out;
    f32 fade_duration; // If set to 0, will default to LIGHT_DEFAULT_FADE_OUT_TIME
    f32 time_elapsed;
} LightSource;

TriangleFan   get_visibility_polygon(Vector2 origin, struct Tilemap *tilemap, struct LinearArena *arena);
void          render_light_source(struct World *world, struct RenderBatch *rb, Vector2 origin,
				  LightSource light, f32 intensity, struct LinearArena *arena);

static inline Vector2 get_light_origin_position(Rectangle entity_bounds)
{
    Vector2 result = rect_center(entity_bounds);

    return result;
}

#endif //LIGHT_H
