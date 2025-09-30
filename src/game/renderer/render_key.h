#ifndef RENDER_KEY_H
#define RENDER_KEY_H

#include "base/utils.h"
#include "base/maths.h"
#include "asset.h"

#define NULL_TEXTURE         ((TextureHandle){(1 << TEXTURE_KEY_BITS) - 1})
#define NULL_FONT            ((FontHandle){(1 << FONT_KEY_BITS) - 1})

#define LAYER_KEY_POSITION   (64u - LAYER_KEY_BITS)
#define LAYER_KEY_BITS       8u
#define SHADER_KEY_POSITION  (LAYER_KEY_POSITION - SHADER_KEY_BITS)
#define SHADER_KEY_BITS      8u
#define TEXTURE_KEY_POSITION (SHADER_KEY_POSITION - TEXTURE_KEY_BITS)
#define TEXTURE_KEY_BITS     16u
#define FONT_KEY_POSITION    (TEXTURE_KEY_POSITION - FONT_KEY_BITS)
#define FONT_KEY_BITS        3u

typedef u64 RenderKey;

typedef enum {
    RENDER_LAYER_TILEMAP,
    RENDER_LAYER_ENTITIES,
    RENDER_LAYER_PARTICLES,
} RenderLayer;

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

inline static RenderKey render_key_create(s32 layer, ShaderHandle shader, TextureHandle texture, FontHandle font,
    s32 y_pos)
{
    (void)y_pos;
    ASSERT(layer < pow(2, LAYER_KEY_BITS));
    ASSERT(shader.id < pow(2, SHADER_KEY_BITS));
    ASSERT(shader.id < pow(2, TEXTURE_KEY_BITS));
    ASSERT(font.id < pow(2, FONT_KEY_BITS));

    RenderKey result = 0;

    result |= (u64)layer      << (LAYER_KEY_POSITION);
    //result |= (u64)y_pos << (Y_POS_KEY_POSITION);
    result |= (u64)shader.id  << (SHADER_KEY_POSITION);
    result |= (u64)texture.id << (TEXTURE_KEY_POSITION);
    result |= (u64)font.id    << (FONT_KEY_POSITION);

    return result;
}

#endif //RENDER_KEY_H
