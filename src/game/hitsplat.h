#ifndef HITSPLAT_H
#define HITSPLAT_H

#include "base/typedefs.h"
#include "base/vector.h"
#include "base/ring_buffer.h"
#include "damage.h"

struct World;
struct LinearArena;
struct RenderBatch;
struct FrameData;

// TODO: make size and velocity depend on damage number
typedef struct {
    TypedDamageValue damage;
    Vector2 position;
    Vector2 velocity;
    f32 timer;
    f32 lifetime;
} Hitsplat;

DEFINE_STATIC_RING_BUFFER(Hitsplat, HitsplatBuffer, 128);

void hitsplats_update(struct World *world, const struct FrameData *frame_data);
void hitsplats_render(struct World *world, struct RenderBatch *rb, struct LinearArena *frame_arena);
void hitsplats_create(struct World *world, Vector2 position, Damage damage);

#endif //HITSPLAT_H
