#include "render_batch.h"
#include "base/linear_arena.h"
#include "base/utils.h"
#include "render_command.h"

#define allocate_render_cmd(arena, kind) init_render_cmd(arena, kind)

static void *allocate_render_cmd(LinearArena *arena, RenderCmdKind kind)
{
    void *result = 0;

    switch (kind) {
        case RENDER_CMD_SPRITE: {
            result = la_allocate_item(arena, SpriteCmd);
        } break;

        case RENDER_CMD_RECTANGLE: {
            result = la_allocate_item(arena, RectangleCmd);
        } break;

        INVALID_DEFAULT_CASE;
    }

    ASSERT(result);
    ((RenderCmdHeader *)result)->kind = kind;

    return result;
}

static RenderEntry *push_render_entry(RenderBatch *rb, RenderKey key, void *data)
{
    RenderEntry *entry = &rb->entries[rb->entry_count++];
    entry->key = key;
    entry->data = data;

    return entry;
}

RenderEntry *render_batch_push_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, ShaderHandle shader, s32 layer)
{
    SpriteCmd *sprite = allocate_render_cmd(arena, RENDER_CMD_SPRITE);
    sprite->rect = rectangle;

    RenderKey key = render_key_create(layer, shader, texture, (s32)rectangle.position.y);
    RenderEntry *result = push_render_entry(rb, key, sprite);

    return result;
}

RenderEntry *render_batch_push_quad(RenderBatch *rb, LinearArena *arena, Rectangle rect,
    RGBA32 color, ShaderHandle shader, s32 layer)
{
    RectangleCmd *cmd = allocate_render_cmd(arena, RENDER_CMD_RECTANGLE);
    cmd->rect = rect;
    cmd->color = color;

    RenderKey key = render_key_create(layer, shader, NULL_TEXTURE, (s32)rect.position.y);
    RenderEntry *result = push_render_entry(rb, key, cmd);

    return result;
}

void render_entry_set_uniform_vec4(RenderEntry *re, LinearArena *arena, String uniform_name, Vector4 vec)
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
