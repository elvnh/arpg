#ifndef RENDER_BATCH_H
#define RENDER_BATCH_H

#include "base/linear_arena.h"
#include "base/vector.h"
#include "render_command.h"
#include "render_key.h"

typedef struct {
    RenderKey         key;
    RenderCmdHeader  *data;
} RenderEntry;

typedef struct {
    RenderEntry  entries[512];
    ssize        entry_count;
} RenderBatch;

RenderEntry *rb_push_sprite(RenderBatch *rb, LinearArena *arena, TextureHandle texture,
    Rectangle rectangle, ShaderHandle shader, s32 layer);

void render_entry_set_uniform_vec4(RenderEntry *re, LinearArena *arena, String uniform_name, Vector4 vec);

#endif //RENDER_BATCH_H
