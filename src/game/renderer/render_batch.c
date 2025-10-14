#include "render_batch.h"
#include "base/linear_arena.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "game/renderer/render_key.h"
#include "render_command.h"
#include "game/particle.h"

RenderBatch *rb_list_push_new(RenderBatchList *list, Matrix4 projection, YDirection y_dir, LinearArena *arena)
{
    RenderBatchNode *node = la_allocate_item(arena, RenderBatchNode);
    list_push_back(list, node);

    node->render_batch.projection = projection;
    node->render_batch.y_direction = y_dir;

    return &node->render_batch;
}

static int sort_cmp(const void *a, const void *b)
{
    const RenderEntry *entry_a = a;
    const RenderEntry *entry_b = b;

    if (entry_a->key < entry_b->key) {
        return -1;
    } else if (entry_a->key > entry_b->key) {
        return 1;
    }

    return 0;
}

void rb_sort_entries(RenderBatch *rb)
{
    // TODO: don't use qsort here, use a stable sort
    qsort(rb->entries, (usize)rb->entry_count, sizeof(*rb->entries), sort_cmp);

    for (ssize i = 0; i < rb->entry_count - 1; ++i) {
        RenderEntry *a = &rb->entries[i];
        RenderEntry *b = &rb->entries[i + 1];

        ASSERT(a->key <= b->key);
    }
}

// TODO: can this be automated?
static void *allocate_render_cmd(LinearArena *arena, RenderCmdKind kind)
{
    void *result = 0;

    switch (kind) {
        case RENDER_CMD_RECTANGLE: {
            result = la_allocate_item(arena, RectangleCmd);
        } break;

        case RENDER_CMD_CLIPPED_RECTANGLE: {
            result = la_allocate_item(arena, ClippedRectangleCmd);
        } break;

        case RENDER_CMD_OUTLINED_RECTANGLE: {
            result = la_allocate_item(arena, OutlinedRectangleCmd);
        } break;

        case RENDER_CMD_CIRCLE: {
            result = la_allocate_item(arena, CircleCmd);
        } break;

        case RENDER_CMD_LINE: {
            result = la_allocate_item(arena, LineCmd);
        } break;

        case RENDER_CMD_TEXT: {
            result = la_allocate_item(arena, TextCmd);
        } break;

        case RENDER_CMD_PARTICLES: {
            result = la_allocate_item(arena, ParticleGroupCmd);
        } break;

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
    Rectangle rectangle, RGBA32 color, ShaderHandle shader, RenderLayer layer)
{
    RectangleCmd *cmd = allocate_render_cmd(arena, RENDER_CMD_RECTANGLE);
    cmd->rect = rectangle;
    cmd->color = color;

    RenderKey key = render_key_create(layer, shader, texture, NULL_FONT, (s32)rectangle.position.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *rb_push_rect(RenderBatch *rb, LinearArena *arena, Rectangle rect,
    RGBA32 color, ShaderHandle shader, RenderLayer layer)
{
    return rb_push_sprite(rb, arena, NULL_TEXTURE, rect, color, shader, layer);
}

RenderEntry *rb_push_clipped_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture, Rectangle rect,
    Rectangle viewport, RGBA32 color, ShaderHandle shader, RenderLayer layer)
{
    ClippedRectangleCmd *cmd = allocate_render_cmd(arena, RENDER_CMD_CLIPPED_RECTANGLE);
    cmd->rect = rect;
    cmd->viewport_rect = viewport;
    cmd->color = color;

    RenderKey key = render_key_create(layer, shader, texture, NULL_FONT, (s32)rect.position.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *rb_push_outlined_rect(RenderBatch *rb, LinearArena *arena, Rectangle rect, RGBA32 color,
    f32 thickness, ShaderHandle shader, RenderLayer layer)
{
    OutlinedRectangleCmd *cmd = allocate_render_cmd(arena, RENDER_CMD_OUTLINED_RECTANGLE);
    cmd->rect = rect;
    cmd->color = color;
    cmd->thickness = thickness;

    RenderKey key = render_key_create(layer, shader, NULL_TEXTURE, NULL_FONT, 0);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *rb_push_sprite_circle(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Vector2 position, RGBA32 color, f32 radius, ShaderHandle shader, RenderLayer layer)
{
    CircleCmd *cmd = allocate_render_cmd(arena, RENDER_CMD_CIRCLE);
    cmd->position = position;
    cmd->color = color;
    cmd->radius = radius;

    RenderKey key = render_key_create(layer, shader, texture, NULL_FONT, (s32)position.y);
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
    LineCmd *cmd = allocate_render_cmd(arena, RENDER_CMD_LINE);
    cmd->start = start;
    cmd->end = end;
    cmd->color = color;
    cmd->thickness = thickness;

    RenderKey key = render_key_create(layer, shader, NULL_TEXTURE, NULL_FONT, (s32)start.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *rb_push_text(RenderBatch *rb, LinearArena *arena, String text, Vector2 position,
    RGBA32 color, s32 size, ShaderHandle shader, FontHandle font, RenderLayer layer)
{
    TextCmd *cmd = allocate_render_cmd(arena, RENDER_CMD_TEXT);
    cmd->text = text;
    cmd->position = position;
    cmd->color = color;
    cmd->size = size;

    RenderKey key = render_key_create(layer, shader, NULL_TEXTURE, font, 0);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *rb_push_particles(RenderBatch *rb, LinearArena *arena, ParticleBuffer *particles,
    RGBA32 color, f32 particle_size, ShaderHandle shader, RenderLayer layer)
{
    ParticleGroupCmd *cmd = allocate_render_cmd(arena, RENDER_CMD_PARTICLES);
    cmd->particles = particles;
    cmd->color = color;
    cmd->particle_size = particle_size;

    ASSERT(particle_size > 0.0f);

    RenderKey key = render_key_create(layer, shader, NULL_TEXTURE, NULL_FONT, 0);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

RenderEntry *rb_push_particles_textured(RenderBatch *rb, LinearArena *arena, struct ParticleBuffer *particles,
    TextureHandle texture, RGBA32 color, f32 particle_size, ShaderHandle shader, RenderLayer layer)
{
    RenderEntry *entry = rb_push_particles(rb, arena, particles, color, particle_size, shader, layer);
    entry->key = render_key_create(layer, shader, texture, NULL_FONT, 0);

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
