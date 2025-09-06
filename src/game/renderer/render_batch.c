#include "render_batch.h"
#include "base/linear_arena.h"
#include "render_command.h"

RenderEntry *rb_push_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, ShaderHandle shader, s32 layer)
{
    SpriteCmd *sprite = la_allocate_item(arena, SpriteCmd);
    sprite->header.kind = RENDER_CMD_SPRITE;
    sprite->rect = rectangle;

    RenderKey key = render_key_create(layer, shader, texture, (s32)rectangle.position.y);
    RenderEntry *entry = &rb->entries[rb->entry_count++];
    entry->key = key;
    entry->data = (RenderCmdHeader *)sprite;

    return entry;
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
