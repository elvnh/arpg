#ifndef RENDER_KEY_H
#define RENDER_KEY_H

#include "assets.h"
#include "base/utils.h"

#define LAYER_KEY_POSITION   (64u - LAYER_KEY_BITS)
#define LAYER_KEY_BITS       8u

#define SHADER_KEY_POSITION  (LAYER_KEY_POSITION - SHADER_KEY_BITS)
#define SHADER_KEY_BITS      8u

#define TEXTURE_KEY_POSITION (SHADER_KEY_POSITION - TEXTURE_KEY_BITS)
#define TEXTURE_KEY_BITS     16u

typedef u64 RenderKey;

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

inline static RenderKey render_key_create(s32 layer, ShaderHandle shader, TextureHandle texture, s32 y_pos)
{
    (void)y_pos;
    // TODO: bounds checks
    RenderKey result = 0;

    result |= (u64)layer      << (LAYER_KEY_POSITION);
    //result |= (u64)y_pos << (Y_POS_KEY_POSITION);
    result |= (u64)shader.id  << (SHADER_KEY_POSITION);
    result |= (u64)texture.id << (TEXTURE_KEY_POSITION);

    return result;
}

#endif //RENDER_KEY_H
