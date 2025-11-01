#include "render_batch.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "game/camera.h"
#include "game/renderer/render_key.h"
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

RenderBatch *rb_list_push_new(RenderBatchList *list, Camera camera, Vector2i viewport_size,
    YDirection y_dir, LinearArena *arena)
{
    RenderBatchNode *node = la_allocate_item(arena, RenderBatchNode);
    list_push_back(list, node);

    Matrix4 proj_matrix = camera_get_matrix(camera, viewport_size, y_dir);

    Rectangle camera_bounds = camera_get_visible_area(camera, viewport_size);
    f32 top_y = camera_bounds.position.y;

    if (y_dir == Y_IS_UP) {
        top_y += camera_bounds.size.y;
    }

    node->render_batch.projection = proj_matrix;
    node->render_batch.y_direction = y_dir;
    node->render_batch.y_sorting_basis = (s32)top_y;

    return &node->render_batch;
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
        b32 left_element_smaller = left[left_idx].key <= right[right_idx].key;

        if (!left_half_done && (left_element_smaller || right_half_done)) {
            dst[dst_idx] = left[left_idx++];
        } else {
            dst[dst_idx] = right[right_idx++];
        }
    }
}

static void merge_sort_render_entries(RenderEntry *entries, ssize count, LinearArena *scratch)
{
    if (count == 1) {

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

void rb_sort_entries(RenderBatch *rb, LinearArena *scratch)
{
    merge_sort_render_entries(rb->entries, rb->entry_count, scratch);

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


RenderEntry *rb_push_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, f32 rotation_in_radians, RectangleFlip flip, RGBA32 color, ShaderHandle shader, RenderLayer layer)
{
    ASSERT(rect_is_valid(rectangle));

    RectangleCmd *cmd = allocate_render_cmd(arena, RectangleCmd);
    cmd->rect = rectangle;
    cmd->color = color;
    cmd->rotation_in_radians = rotation_in_radians;
    cmd->flip = flip;

    // TODO: make push_render_entry call render_key_create
    RenderKey key = render_key_create(rb, layer, shader, texture, NULL_FONT, rectangle.position.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *rb_push_rect(RenderBatch *rb, LinearArena *arena, Rectangle rect,
    RGBA32 color, ShaderHandle shader, RenderLayer layer)
{
    return rb_push_sprite(rb, arena, NULL_TEXTURE, rect, 0.0f, RECT_FLIP_NONE, color, shader, layer);
}

RenderEntry *rb_push_clipped_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture, Rectangle rect,
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

RenderEntry *rb_push_outlined_rect(RenderBatch *rb, LinearArena *arena, Rectangle rect, RGBA32 color,
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

RenderEntry *rb_push_sprite_circle(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
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

RenderEntry *rb_push_circle(RenderBatch *rb, LinearArena *arena, Vector2 position,
    RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer)
{
    return rb_push_sprite_circle(rb, arena, NULL_TEXTURE, position, color, radius, shader, layer);
}

RenderEntry *rb_push_line(RenderBatch *rb, LinearArena *arena, Vector2 start, Vector2 end,
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

RenderEntry *rb_push_text(RenderBatch *rb, LinearArena *arena, String text, Vector2 position,
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

RenderEntry *rb_push_clipped_text(RenderBatch *rb, LinearArena *arena, String text, Vector2 position,
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

RenderEntry *rb_push_particles(RenderBatch *rb, LinearArena *arena, ParticleBuffer *particles,
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

RenderEntry *rb_push_particles_textured(RenderBatch *rb, LinearArena *arena, struct ParticleBuffer *particles,
    TextureHandle texture, RGBA32 color, f32 particle_size, ShaderHandle shader, RenderLayer layer)
{
    RenderEntry *entry = rb_push_particles(rb, arena, particles, color, particle_size, shader, layer);
    entry->key = render_key_create(rb, layer, shader, texture, NULL_FONT, 0);

    return entry;
}

void re_set_uniform_vec4(RenderEntry *re, LinearArena *arena, String uniform_name, Vector4 vec)
{
    SetupCmdUniformVec4 *cmd = la_allocate_item(arena, SetupCmdUniformVec4);
    cmd->header.kind = SETUP_CMD_SET_UNIFORM_VEC4;
    cmd->uniform_name = uniform_name;
    cmd->vector = vec;

    if (!re->data->first_setup_command) {
        re->data->first_setup_command = (SetupCmdHeader *)cmd;
    } else {
        SetupCmdHeader *curr_cmd;
        for (curr_cmd = re->data->first_setup_command; curr_cmd->next; curr_cmd = curr_cmd->next);

        curr_cmd->next = (SetupCmdHeader *)cmd;
    }
}
