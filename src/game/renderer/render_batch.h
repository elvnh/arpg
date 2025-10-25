#ifndef RENDER_BATCH_H
#define RENDER_BATCH_H

#include "base/linear_arena.h"
#include "base/vector.h"
#include "base/matrix.h"
#include "base/list.h"
#include "render_command.h"
#include "render_key.h"

// TODO: make RenderEntry array into ring buffer in case it overflows
// TODO: reduce amount of parameters, eg. create specialization for push_sprite in case
// color needs to be provided, otherwise default to white

typedef struct RenderEntry {
    RenderKey         key;
    RenderCmdHeader  *data;
} RenderEntry;

typedef struct RenderBatch {
    RenderEntry  entries[1024];
    ssize        entry_count;
    Matrix4      projection;
    YDirection   y_direction; // TODO: get this from projection matrix instead of storing
} RenderBatch;

typedef struct RenderBatchNode {
    RenderBatch  render_batch;
    LIST_LINKS(RenderBatchNode);
} RenderBatchNode;

// TODO: this doesn't need to be doubly linked
DEFINE_LIST(RenderBatchNode, RenderBatchList);

struct Particle;

RenderBatch *rb_list_push_new(RenderBatchList *list, Matrix4 projection, YDirection y_dir, LinearArena *arena);
void         rb_sort_entries(RenderBatch *rb, LinearArena *scratch);
RenderEntry *rb_push_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, RGBA32 color, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_rect(RenderBatch *rb, LinearArena *arena, Rectangle rect, RGBA32 color,
    ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_clipped_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rect, Rectangle viewport, RGBA32 color, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_outlined_rect(RenderBatch *rb, LinearArena *arena, Rectangle rect, RGBA32 color,
    f32 thickness, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_sprite_circle(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Vector2 position, RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_circle(RenderBatch *rb, LinearArena *arena, Vector2 position,
    RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_line(RenderBatch *rb, LinearArena *arena, Vector2 start, Vector2 end,
    RGBA32 color, f32 thickness, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_text(RenderBatch *rb, LinearArena *arena, String text, Vector2 position,
    RGBA32 color, s32 size, ShaderHandle shader, FontHandle font, RenderLayer layer);
RenderEntry *rb_push_clipped_text(RenderBatch *rb, LinearArena *arena, String text, Vector2 position,
    Rectangle clip_rect, RGBA32 color, s32 size, ShaderHandle shader, FontHandle font, RenderLayer layer);
RenderEntry *rb_push_particles(RenderBatch *rb, LinearArena *arena, struct ParticleBuffer *particles,
    RGBA32 color, f32 particle_size, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_particles_textured(RenderBatch *rb, LinearArena *arena, struct ParticleBuffer *particles,
    TextureHandle texture, RGBA32 color, f32 particle_size, ShaderHandle shader, RenderLayer layer);

void re_set_uniform_vec4(RenderEntry *re, LinearArena *arena, String uniform_name, Vector4 vec);

#endif //RENDER_BATCH_H
