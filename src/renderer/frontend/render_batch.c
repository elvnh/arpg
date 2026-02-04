#include "render_batch.h"
#include "base/linear_arena.h"
#include "base/list.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "game/camera.h"
#include "renderer/frontend/render_key.h"
#include "render_command.h"
#include "game/components/particle.h"

// TODO: lots of code duplication in this file

#define allocate_render_cmd(arena, type) (type *)(allocate_render_cmd_impl((arena), RENDER_COMMAND_ENUM_NAME(type)))

static inline s32 get_key_y_sort_order(struct RenderBatch *rb, f32 y_pos)
{
    s32 result = 0;

    if (rb->y_direction == Y_IS_UP) {
        result = rb->y_sorting_basis - (s32)y_pos;
    } else {
        result = (s32)y_pos - rb->y_sorting_basis;
    }

    return result;
}

static inline RenderKey render_key_create(struct RenderBatch *rb, s32 layer, ShaderHandle shader, TextureHandle texture,
    FontHandle font, f32 y_pos)
{
    ASSERT(rb);
    ASSERT(layer < pow(2, LAYER_KEY_BITS));
    ASSERT(shader.id < pow(2, SHADER_KEY_BITS));
    ASSERT(shader.id < pow(2, TEXTURE_KEY_BITS));
    ASSERT(font.id < pow(2, FONT_KEY_BITS));

    RenderKey result = 0;
    s32 y_order = get_key_y_sort_order(rb, y_pos);
    ASSERT(y_order < pow(2, Y_ORDER_KEY_BITS));

    result |= (u64)layer      << (LAYER_KEY_POSITION);
    result |= (u64)y_order    << (Y_ORDER_KEY_POSITION);
    result |= (u64)shader.id  << (SHADER_KEY_POSITION);
    result |= (u64)texture.id << (TEXTURE_KEY_POSITION);
    result |= (u64)font.id    << (FONT_KEY_POSITION);

    return result;
}

static RenderBatchNode *rb_create_node(LinearArena *arena)
{
    RenderBatchNode *node = la_allocate_item(arena, RenderBatchNode);

    return node;
}

static RenderBatch rb_create(Camera camera, Vector2i viewport_size, YDirection y_dir, FrameBuffer render_target,
    RGBA32 clear_color, BlendFunction blend_func)
{
    RenderBatch result = {0};

    Matrix4 proj_matrix = camera_get_matrix(camera, viewport_size, y_dir);

    Rectangle camera_bounds = camera_get_visible_area(camera, viewport_size);
    f32 top_y = camera_bounds.position.y;

    if (y_dir == Y_IS_UP) {
        top_y += camera_bounds.size.y;
    }

    result.projection = proj_matrix;
    result.y_direction = y_dir;
    result.y_sorting_basis = (s32)top_y;
    result.render_target = render_target;
    result.clear_color = clear_color;
    result.blend_function = blend_func;

    return result;
}

RenderBatch *push_new_render_batch(RenderBatchList *list, Camera camera, Vector2i viewport_size, YDirection y_dir,
    FrameBuffer render_target, RGBA32 clear_color, BlendFunction blend_func, LinearArena *arena)
{
    RenderBatchNode *node = rb_create_node(arena);
    node->render_batch = rb_create(camera, viewport_size, y_dir, render_target, clear_color, blend_func);

    list_push_back(list, node);

    return &node->render_batch;
}

RenderBatch *add_stencil_pass(RenderBatch *rb, StencilFunction stencil_func, s32 stencil_func_arg,
    StencilOperation stencil_op, LinearArena *arena)
{
    RenderBatch *stencil_batch = la_allocate_item(arena, RenderBatch);
    stencil_batch->y_sorting_basis = rb->y_sorting_basis;
    stencil_batch->projection = rb->projection;
    stencil_batch->y_direction = rb->y_direction;
    stencil_batch->render_target = rb->render_target;
    stencil_batch->clear_color = rb->clear_color;
    stencil_batch->blend_function = rb->blend_function;

    stencil_batch->stencil_func = stencil_func;
    stencil_batch->stencil_func_arg = stencil_func_arg;
    stencil_batch->stencil_op = stencil_op;

    rb->stencil_batch = stencil_batch;

    return stencil_batch;
}

static void merge_render_entry_arrays(RenderEntry *dst, RenderEntry *left, ssize left_count,
    RenderEntry *right, ssize right_count)
{
    ssize end = left_count + right_count;

    ssize left_idx = 0;
    ssize right_idx = 0;

    for (ssize dst_idx = 0; dst_idx < end; ++dst_idx) {
        b32 left_half_done = left_idx == left_count;
        b32 right_half_done = right_idx == right_count;

	ASSERT(!(left_half_done && right_half_done));

        if (!left_half_done && (right_half_done || (left[left_idx].key <= right[right_idx].key))) {
	    ASSERT(left_idx < left_count);

            dst[dst_idx] = left[left_idx++];
        } else {
	    ASSERT(!right_half_done);
	    ASSERT(right_idx < right_count);

            dst[dst_idx] = right[right_idx++];
        }
    }
}

static void merge_sort_render_entries(RenderEntry *entries, ssize count, LinearArena *scratch)
{
    if (count <= 1) {

    } else if (count == 2) {
        RenderEntry *a = &entries[0];
        RenderEntry *b = &entries[1];

        if (a->key > b->key) {
            RenderEntry tmp = *a;
            *a = *b;
            *b = tmp;
        }
    } else {
        RenderEntry *copy = la_copy_array(scratch, entries, count);

        ssize left_half_count = count / 2;
        ssize right_half_count = count - left_half_count;

        RenderEntry *left_half = copy;
        RenderEntry *right_half = left_half + left_half_count;

        merge_sort_render_entries(left_half, left_half_count, scratch);
        merge_sort_render_entries(right_half, right_half_count, scratch);

        merge_render_entry_arrays(entries, left_half, left_half_count, right_half, right_half_count);
    }
}

void sort_render_entries(RenderBatch *rb, LinearArena *scratch)
{
    merge_sort_render_entries(rb->entries, rb->entry_count, scratch);

    // TODO: use a unit test for this instead
#if 1
    for (ssize i = 0; i < rb->entry_count - 1; ++i) {
        RenderEntry *a = &rb->entries[i];
        RenderEntry *b = &rb->entries[i + 1];

        ASSERT(a->key <= b->key);
    }
#endif
}

static void *allocate_render_cmd_impl(LinearArena *arena, RenderCmdKind kind)
{
    void *result = 0;

    switch (kind) {
#define RENDER_COMMAND(type) case RENDER_COMMAND_ENUM_NAME(type): { result = la_allocate_item(arena, type); } break;
        RENDER_COMMAND_LIST
#undef RENDER_COMMAND

        INVALID_DEFAULT_CASE;
    }

    ASSERT(result);
    ((RenderCmdHeader *)result)->kind = kind;

    return result;
}

static RenderEntry *push_render_entry(RenderBatch *rb, RenderKey key, void *data)
{
    ASSERT(rb->entry_count < ARRAY_COUNT(rb->entries));
    RenderEntry *entry = &rb->entries[rb->entry_count++];
    entry->key = key;
    entry->data = data;

    return entry;
}

RenderEntry *draw_colored_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, SpriteModifiers mods, RGBA32 color,
    ShaderHandle shader, RenderLayer layer)
{
    ASSERT(rect_is_valid(rectangle));

    RectangleCmd *cmd = allocate_render_cmd(arena, RectangleCmd);
    cmd->rect = rectangle;
    cmd->color = color;
    cmd->rotation_in_radians = mods.rotation;
    cmd->flip = mods.flip;

    // TODO: make push_render_entry call render_key_create
    RenderKey key = render_key_create(rb, layer, shader, texture, NULL_FONT, rectangle.position.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, SpriteModifiers mods, ShaderHandle shader, RenderLayer layer)
{
    return draw_colored_sprite(rb, arena, texture, rectangle, mods,
	RGBA32_WHITE, shader, layer);
}

RenderEntry *draw_rectangle(RenderBatch *rb, LinearArena *arena, Rectangle rect,
    RGBA32 color, ShaderHandle shader, RenderLayer layer)
{
    return draw_colored_sprite(rb, arena, NULL_TEXTURE, rect, (SpriteModifiers){0}, color, shader, layer);
}

RenderEntry *draw_triangle(RenderBatch *rb, LinearArena *arena, Triangle triangle, RGBA32 color,
    ShaderHandle shader, RenderLayer layer)
{
    TriangleCmd *cmd = allocate_render_cmd(arena, TriangleCmd);
    cmd->triangle = triangle;
    cmd->color = color;

    RenderKey key = render_key_create(rb, layer, shader, NULL_TEXTURE, NULL_FONT, triangle.a.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_outlined_triangle(RenderBatch *rb, LinearArena *arena, Triangle triangle, RGBA32 color,
    f32 thickness, ShaderHandle shader, RenderLayer layer)
{
    OutlinedTriangleCmd *cmd = allocate_render_cmd(arena, OutlinedTriangleCmd);
    cmd->triangle = triangle;
    cmd->color = color;
    cmd->thickness = thickness;

    RenderKey key = render_key_create(rb, layer, shader, NULL_TEXTURE, NULL_FONT, triangle.a.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_clipped_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture, Rectangle rect,
    Rectangle viewport, RGBA32 color, ShaderHandle shader, RenderLayer layer)
{
    ClippedRectangleCmd *cmd = allocate_render_cmd(arena, ClippedRectangleCmd);
    cmd->rect = rect;
    cmd->viewport_rect = viewport;
    cmd->color = color;

    RenderKey key = render_key_create(rb, layer, shader, texture, NULL_FONT, rect.position.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_outlined_rectangle(RenderBatch *rb, LinearArena *arena, Rectangle rect, RGBA32 color,
    f32 thickness, ShaderHandle shader, RenderLayer layer)
{
    OutlinedRectangleCmd *cmd = allocate_render_cmd(arena, OutlinedRectangleCmd);
    cmd->rect = rect;
    cmd->color = color;
    cmd->thickness = thickness;

    RenderKey key = render_key_create(rb, layer, shader, NULL_TEXTURE, NULL_FONT, 0);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_textured_circle(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Vector2 position, RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer)
{
    CircleCmd *cmd = allocate_render_cmd(arena, CircleCmd);
    cmd->position = position;
    cmd->color = color;
    cmd->radius = radius;

    RenderKey key = render_key_create(rb, layer, shader, texture, NULL_FONT, (s32)position.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_circle(RenderBatch *rb, LinearArena *arena, Vector2 position,
    RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer)
{
    return draw_textured_circle(rb, arena, NULL_TEXTURE, position, color, radius, shader, layer);
}

RenderEntry *draw_line(RenderBatch *rb, LinearArena *arena, Vector2 start, Vector2 end,
    RGBA32 color, f32 thickness, ShaderHandle shader, RenderLayer layer)
{
    LineCmd *cmd = allocate_render_cmd(arena, LineCmd);
    cmd->start = start;
    cmd->end = end;
    cmd->color = color;
    cmd->thickness = thickness;

    RenderKey key = render_key_create(rb, layer, shader, NULL_TEXTURE, NULL_FONT, (s32)start.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_text(RenderBatch *rb, LinearArena *arena, String text, Vector2 position,
    RGBA32 color, s32 size, ShaderHandle shader, FontHandle font, RenderLayer layer)
{
    TextCmd *cmd = allocate_render_cmd(arena, TextCmd);
    cmd->text = text;
    cmd->position = position;
    cmd->color = color;
    cmd->size = size;

    RenderKey key = render_key_create(rb, layer, shader, NULL_TEXTURE, font, 0);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_clipped_text(RenderBatch *rb, LinearArena *arena, String text, Vector2 position,
    Rectangle clip_rect, RGBA32 color, s32 size, ShaderHandle shader, FontHandle font, RenderLayer layer)
{
    TextCmd *cmd = allocate_render_cmd(arena, TextCmd);
    cmd->text = text;
    cmd->position = position;
    cmd->color = color;
    cmd->size = size;
    cmd->is_clipped = true;
    cmd->clip_rect = clip_rect;

    RenderKey key = render_key_create(rb, layer, shader, NULL_TEXTURE, font, 0);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_particles(RenderBatch *rb, LinearArena *arena, ParticleBuffer *particles,
    RGBA32 color, f32 particle_size, ShaderHandle shader, RenderLayer layer)
{
    ParticleGroupCmd *cmd = allocate_render_cmd(arena, ParticleGroupCmd);
    cmd->particles = particles;
    cmd->color = color;
    cmd->particle_size = particle_size;

    ASSERT(particle_size > 0.0f);

    RenderKey key = render_key_create(rb, layer, shader, NULL_TEXTURE, NULL_FONT, 0);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_textured_particles(RenderBatch *rb, LinearArena *arena, struct ParticleBuffer *particles,
    TextureHandle texture, RGBA32 color, f32 particle_size, ShaderHandle shader, RenderLayer layer)
{
    RenderEntry *entry = draw_particles(rb, arena, particles, color, particle_size, shader, layer);
    entry->key = render_key_create(rb, layer, shader, texture, NULL_FONT, 0);

    return entry;
}

RenderEntry *draw_polygon(RenderBatch *rb, LinearArena *arena, TriangulatedPolygon polygon,
    RGBA32 color, ShaderHandle shader, RenderLayer layer)
{
    PolygonCmd *cmd = allocate_render_cmd(arena, PolygonCmd);
    cmd->polygon = polygon;
    cmd->color = color;

    RenderKey key = render_key_create(rb, layer, shader, NULL_TEXTURE, NULL_FONT, 0);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *draw_triangle_fan(RenderBatch *rb, LinearArena *arena, TriangleFan triangle_fan,
    RGBA32 color, ShaderHandle shader, RenderLayer layer)
{
    TriangleFanCmd *cmd = allocate_render_cmd(arena, TriangleFanCmd);
    cmd->triangle_fan = triangle_fan;
    cmd->color = color;

    RenderKey key = render_key_create(rb, layer, shader, NULL_TEXTURE, NULL_FONT, 0);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}



#define allocate_render_setup_cmd(arena, entry, type, uniform_name)      \
    (type *)(allocate_render_setup_cmd_impl((arena), entry, RENDER_SETUP_COMMAND_ENUM_NAME(type), uniform_name))

static void *allocate_render_setup_cmd_impl(LinearArena *arena, RenderEntry *re,
    SetupCmdKind kind, String uniform_name)
{
    void *result = 0;

    switch (kind) {
#define RENDER_SETUP_COMMAND(type)                                      \
        case RENDER_SETUP_COMMAND_ENUM_NAME(type): { result = la_allocate_item(arena, type); } break;

        RENDER_SETUP_COMMAND_LIST
#undef RENDER_SETUP_COMMAND

        INVALID_DEFAULT_CASE;
    }

    ASSERT(result);

    SetupCmdHeader *header = result;
    header->kind = kind;
    header->uniform_name = uniform_name;

    if (!re->data->first_setup_command) {
        re->data->first_setup_command = header;
    } else {
        SetupCmdHeader *curr_cmd;
        for (curr_cmd = re->data->first_setup_command; curr_cmd->next; curr_cmd = curr_cmd->next);

        curr_cmd->next = header;
    }

    return result;
}

void re_set_uniform_vec4(RenderEntry *re, LinearArena *arena, String uniform_name, Vector4 vec)
{
    ASSERT(re);

    SetupCmdUniformVec4 *cmd = allocate_render_setup_cmd(arena, re, SetupCmdUniformVec4, uniform_name);
    cmd->value = vec;
}

void re_set_uniform_float(RenderEntry *re, LinearArena *arena, String uniform_name, f32 value)
{
    ASSERT(re);

    SetupCmdUniformFloat *cmd = allocate_render_setup_cmd(arena, re, SetupCmdUniformFloat, uniform_name);
    cmd->value = value;
}
