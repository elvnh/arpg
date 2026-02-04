#ifndef RENDERER_DISPATCH_H
#define RENDERER_DISPATCH_H

#include "renderer/frontend/render_batch.h"
#include "platform/asset_system.h"

void execute_render_commands(RenderBatch *rb, AssetSystem *assets,
    struct RendererBackend *backend, LinearArena *scratch);

#endif //RENDERER_DISPATCH_H
