#ifndef RENDERER_DISPATCH_H
#define RENDERER_DISPATCH_H

#include "game/renderer/render_batch.h"
#include "asset_manager.h"

void execute_render_commands(RenderBatch *rb, AssetManager *assets,
    struct RendererBackend *backend, LinearArena *scratch);

#endif //RENDERER_DISPATCH_H
