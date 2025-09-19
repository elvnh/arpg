#ifndef RENDER_BATCH_H
#define RENDER_BATCH_H

#include "base/linear_arena.h"
#include "base/vector.h"
#include "base/matrix.h"
#include "render_command.h"
#include "render_key.h"

typedef struct {
    RenderKey         key;
    RenderCmdHeader  *data;
} RenderEntry;

typedef struct {
    RenderEntry  entries[512];
    ssize        entry_count;
    Matrix4      projection;
} RenderBatch;

void render_batch_sort(RenderBatch *rb);
RenderEntry *rb_push_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, RGBA32 color, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_rect(RenderBatch *rb, LinearArena *arena, Rectangle rect, RGBA32 color,
    ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_sprite_circle(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Vector2 position, RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_circle(RenderBatch *rb, LinearArena *arena, Vector2 position,
    RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_line(RenderBatch *rb, LinearArena *arena, Vector2 start, Vector2 end,
    RGBA32 color, f32 thickness, ShaderHandle shader, RenderLayer layer);

void re_set_uniform_vec4(RenderEntry *re, LinearArena *arena, String uniform_name, Vector4 vec);

#endif //RENDER_BATCH_H
