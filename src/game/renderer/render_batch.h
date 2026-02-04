#ifndef RENDER_BATCH_H
#define RENDER_BATCH_H

#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/vector.h"
#include "base/matrix.h"
#include "base/list.h"
#include "camera.h"
#include "render_command.h"
#include "render_key.h"
#include "renderer/renderer_backend.h"
#include "sprite.h"
#include "renderer/render_target.h"

// TODO: make RenderEntry array into ring buffer in case it overflows
// TODO: reduce amount of parameters, eg. create specialization for push_sprite in case
// color needs to be provided, otherwise default to white

typedef struct RenderEntry {
    RenderKey         key;
    RenderCmdHeader  *data;
} RenderEntry;

typedef struct RenderBatch {
    RenderEntry   entries[4096];
    ssize         entry_count;
    s32           y_sorting_basis;
    Matrix4       projection;
    YDirection    y_direction; // TODO: get this from projection matrix instead of storing

    FrameBuffer      render_target;

    RGBA32        clear_color;
    BlendFunction blend_function;

    struct RenderBatch *stencil_batch;
    StencilFunction  stencil_func;
    s32              stencil_func_arg;
    StencilOperation stencil_op;
} RenderBatch;

// TODO: just store links in RenderBatch directly?
typedef struct RenderBatchNode {
    RenderBatch  render_batch;
    LIST_LINKS(RenderBatchNode);
} RenderBatchNode;

// TODO: this doesn't need to be doubly linked
DEFINE_LIST(RenderBatchNode, RenderBatchList);

struct Particle;

RenderBatch *rb_list_push_new(RenderBatchList *list, Camera camera, Vector2i viewport_size, YDirection y_dir,
    FrameBuffer render_target, RGBA32 clear_color, BlendFunction blend_func, LinearArena *arena);
RenderBatch *rb_add_stencil_pass(RenderBatch *rb, StencilFunction stencil_func, s32 stencil_func_arg,
                                 StencilOperation stencil_op, LinearArena *arena);
void         rb_sort_entries(RenderBatch *rb, LinearArena *scratch);
RenderEntry *rb_push_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, SpriteModifiers mods, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_colored_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, SpriteModifiers mods, RGBA32 color, // TODO: pack color into sprite mods
    ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_rect(RenderBatch *rb, LinearArena *arena, Rectangle rect, RGBA32 color,
    ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_triangle(RenderBatch *rb, LinearArena *arena, Triangle triangle, RGBA32 color,
    ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_outlined_triangle(RenderBatch *rb, LinearArena *arena, Triangle triangle, RGBA32 color,
    f32 thickness, ShaderHandle shader, RenderLayer layer);
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
RenderEntry *rb_push_polygon(RenderBatch *rb, LinearArena *arena, TriangulatedPolygon polygon,
    RGBA32 color, ShaderHandle shader, RenderLayer layer);
RenderEntry *rb_push_triangle_fan(RenderBatch *rb, LinearArena *arena, TriangleFan triangle_fan,
    RGBA32 color, ShaderHandle shader, RenderLayer layer);

void re_set_uniform_vec4(RenderEntry *re, LinearArena *arena, String uniform_name, Vector4 vec);
void re_set_uniform_float(RenderEntry *re, LinearArena *arena, String uniform_name, f32 value);

#endif //RENDER_BATCH_H
