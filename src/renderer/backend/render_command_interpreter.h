#ifndef RENDERER_DISPATCH_H
#define RENDERER_DISPATCH_H

#include "renderer/frontend/render_batch.h"

void execute_render_commands(RenderBatch *rb, struct RendererBackend *backend, LinearArena *scratch);

#endif //RENDERER_DISPATCH_H
