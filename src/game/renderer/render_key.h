#ifndef RENDER_KEY_H
#define RENDER_KEY_H

#include "base/utils.h"
#include "base/maths.h"
#include "asset.h"

#define LAYER_KEY_POSITION   (64u - LAYER_KEY_BITS)
#define LAYER_KEY_BITS       8u

#define Y_ORDER_KEY_POSITION (LAYER_KEY_POSITION - Y_ORDER_KEY_BITS)
#define Y_ORDER_KEY_BITS      14u

#define SHADER_KEY_POSITION  (Y_ORDER_KEY_POSITION - SHADER_KEY_BITS)
#define SHADER_KEY_BITS      14u

#define TEXTURE_KEY_POSITION (SHADER_KEY_POSITION - TEXTURE_KEY_BITS)
#define TEXTURE_KEY_BITS     14u

#define FONT_KEY_POSITION    (TEXTURE_KEY_POSITION - FONT_KEY_BITS)
#define FONT_KEY_BITS        14u

struct RenderBatch;

typedef u64 RenderKey;

typedef enum {
    RENDER_LAYER_FLOORS,
    RENDER_LAYER_PARTICLES,
    RENDER_LAYER_ENTITIES,
    RENDER_LAYER_WALLS,
    RENDER_LAYER_OVERLAY,
} RenderLayer;

// TODO: these should return the asset handles directly so caller doesn't have to create them
inline static RenderKey render_key_extract_shader(RenderKey key)
{
    RenderKey result = bit_span(key, SHADER_KEY_POSITION, SHADER_KEY_BITS);

    return result;
}

inline static RenderKey render_key_extract_texture(RenderKey key)
{
    RenderKey result = bit_span(key, TEXTURE_KEY_POSITION, TEXTURE_KEY_BITS);

    return result;
}

inline static RenderKey render_key_extract_font(RenderKey key)
{
    RenderKey result = bit_span(key, FONT_KEY_POSITION, FONT_KEY_BITS);

    return result;
}

#endif //RENDER_KEY_H
