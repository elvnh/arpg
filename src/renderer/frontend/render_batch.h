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
#include "renderer/backend/renderer_backend.h"
#include "sprite.h"
#include "renderer/frontend/render_target.h"

/*
  TODO:
  - make RenderEntry array into ring buffer in case it overflows?
  - reduce amount of parameters, eg. create specialization for push_sprite in case
    color needs to be provided, otherwise default to white
  - Maybe it would be better to have all types of render commands in union inside renderentry?
  - Just store list links in renderbatch directly
  - RenderBatchList doesn't need to be doubly linked?
  - Get y direction from RenderBatch projection matrix instead of storing
 */

typedef struct RenderEntry {
    RenderKey         key;
    RenderCmdHeader  *data;
} RenderEntry;

typedef struct RenderBatch {
    RenderEntry         entries[4096];
    ssize               entry_count;
    s32                 y_sorting_basis;
    Matrix4             projection;
    YDirection          y_direction;

    FrameBuffer         render_target;

    RGBA32              clear_color;
    BlendFunction       blend_function;

    struct RenderBatch *stencil_batch;
    StencilFunction     stencil_func;
    s32                 stencil_func_arg;
    StencilOperation    stencil_op;
} RenderBatch;

typedef struct RenderBatchNode {
    RenderBatch  render_batch;
    LIST_LINKS(RenderBatchNode);
} RenderBatchNode;

DEFINE_LIST(RenderBatchNode, RenderBatchList);

struct Particle;

RenderBatch *push_new_render_batch(RenderBatchList *list, Camera camera, Vector2i viewport_size, YDirection y_dir,
    FrameBuffer render_target, RGBA32 clear_color, BlendFunction blend_func, LinearArena *arena);
RenderBatch *add_stencil_pass(RenderBatch *rb, StencilFunction stencil_func, s32 stencil_func_arg,
                                 StencilOperation stencil_op, LinearArena *arena);
void         sort_render_entries(RenderBatch *rb, LinearArena *scratch);

RenderEntry *draw_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, SpriteModifiers mods, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_colored_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, SpriteModifiers mods, RGBA32 color, // TODO: pack color into sprite mods
    ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_rectangle(RenderBatch *rb, LinearArena *arena, Rectangle rect, RGBA32 color,
    ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_triangle(RenderBatch *rb, LinearArena *arena, Triangle triangle, RGBA32 color,
    ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_outlined_triangle(RenderBatch *rb, LinearArena *arena, Triangle triangle, RGBA32 color,
    f32 thickness, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_clipped_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rect, Rectangle viewport, RGBA32 color, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_outlined_rectangle(RenderBatch *rb, LinearArena *arena, Rectangle rect, RGBA32 color,
    f32 thickness, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_textured_circle(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Vector2 position, RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_circle(RenderBatch *rb, LinearArena *arena, Vector2 position,
    RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_line(RenderBatch *rb, LinearArena *arena, Vector2 start, Vector2 end,
    RGBA32 color, f32 thickness, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_text(RenderBatch *rb, LinearArena *arena, String text, Vector2 position,
    RGBA32 color, s32 size, ShaderHandle shader, FontHandle font, RenderLayer layer);
RenderEntry *draw_clipped_text(RenderBatch *rb, LinearArena *arena, String text, Vector2 position,
    Rectangle clip_rect, RGBA32 color, s32 size, ShaderHandle shader, FontHandle font, RenderLayer layer);
RenderEntry *draw_particles(RenderBatch *rb, LinearArena *arena, struct ParticleBuffer *particles,
    RGBA32 color, f32 particle_size, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_textured_particles(RenderBatch *rb, LinearArena *arena, struct ParticleBuffer *particles,
    TextureHandle texture, RGBA32 color, f32 particle_size, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_polygon(RenderBatch *rb, LinearArena *arena, TriangulatedPolygon polygon,
    RGBA32 color, ShaderHandle shader, RenderLayer layer);
RenderEntry *draw_triangle_fan(RenderBatch *rb, LinearArena *arena, TriangleFan triangle_fan,
    RGBA32 color, ShaderHandle shader, RenderLayer layer);

void re_set_uniform_vec4(RenderEntry *re, LinearArena *arena, String uniform_name, Vector4 vec);
void re_set_uniform_float(RenderEntry *re, LinearArena *arena, String uniform_name, f32 value);

#endif //RENDER_BATCH_H
