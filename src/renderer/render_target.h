#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

struct RenderBatch;

// TODO: this doesn't need to be a separate file

typedef enum {
    FRAME_BUFFER_GAMEPLAY,
    FRAME_BUFFER_LIGHTING,
    FRAME_BUFFER_OVERLAY,
    FRAME_BUFFER_COUNT,
} FrameBuffer;

typedef enum {
    BLEND_FUNCTION_MULTIPLICATIVE,
    BLEND_FUNCTION_ADDITIVE,
} BlendFunction;

typedef struct {
    struct RenderBatch *world_rb;
    struct RenderBatch *overlay_rb;
    struct RenderBatch *lighting_rb;
    struct RenderBatch *lighting_stencil_rb;
    struct RenderBatch *worldspace_ui_rb;
} RenderBatches;

#endif //RENDER_TARGET_H
