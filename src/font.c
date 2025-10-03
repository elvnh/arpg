#include <stb_truetype.h>

#include "font.h"
#include "asset_manager.h"
#include "renderer/renderer_backend.h"

// TODO: dynamically decide size of atlas
#define FONT_ATLAS_WIDTH             2048
#define FONT_ATLAS_HEIGHT            2048
#define FONT_FIRST_CHAR              ' '
#define FONT_LAST_CHAR               '~'
#define FONT_CHAR_COUNT              (FONT_LAST_CHAR - FONT_FIRST_CHAR)
#define FONT_RASTERIZED_SIZE         128

struct FontAsset {
    TextureHandle        texture_handle;
    stbtt_fontinfo       font_info;
    stbtt_packedchar     glyph_metrics[FONT_CHAR_COUNT];
    stbtt_aligned_quad   glyph_quads[FONT_CHAR_COUNT];
};

// TODO: Most calculations in this file are pretty hacky, fix them

FontAsset *font_create_atlas(Span font_file_contents, struct AssetManager *assets, Allocator allocator,
    LinearArena *scratch)
{
    FontAsset *result = allocate_item(allocator, FontAsset);

    if (!stbtt_InitFont(&result->font_info, font_file_contents.data, 0)) {
        goto error;
    }

    byte *mono_font_bitmap = la_allocate_array(scratch, byte, FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT);

    stbtt_pack_context pack_ctx;
    if (!stbtt_PackBegin(&pack_ctx, mono_font_bitmap, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, 1, 0)) {
        goto error;
    }

    stbtt_PackSetOversampling(&pack_ctx, 2, 2);

    if (!stbtt_PackFontRange(&pack_ctx, font_file_contents.data, 0, FONT_RASTERIZED_SIZE,
            FONT_FIRST_CHAR, FONT_CHAR_COUNT, result->glyph_metrics)) {
        goto error;
    }

    stbtt_PackEnd(&pack_ctx);

    f32 unused_x, unused_y;
    for (s32 i = 0; i < FONT_CHAR_COUNT; ++i) {
        stbtt_GetPackedQuad(result->glyph_metrics, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT,
            i, &unused_x, &unused_y, &result->glyph_quads[i], 0);
    }

    byte *rgba_font_bitmap = la_allocate_array(scratch, byte, FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * 4);

    for (s32 y = 0; y < FONT_ATLAS_HEIGHT; ++y) {
        for (s32 x = 0; x < FONT_ATLAS_WIDTH; ++x) {
            s32 dst_index = (x + y * FONT_ATLAS_WIDTH) * 4;
            s32 src_index = x + y * FONT_ATLAS_WIDTH;
            byte color = mono_font_bitmap[src_index];

            rgba_font_bitmap[dst_index + 0] = color;
            rgba_font_bitmap[dst_index + 1] = color;
            rgba_font_bitmap[dst_index + 2] = color;
            rgba_font_bitmap[dst_index + 3] = color;
        }
    }

    Image font_image = {
        .data = rgba_font_bitmap,
        .width = FONT_ATLAS_WIDTH,
        .height = FONT_ATLAS_HEIGHT,
        .channels = 4
    };

    result->texture_handle = assets_create_texture_from_memory(assets, font_image);

    return result;

  error:
    ASSERT(0);

    font_destroy_atlas(result, allocator);

    return 0;
}

void font_destroy_atlas(FontAsset *asset, Allocator allocator)
{
    deallocate(allocator, asset);
}

TextureHandle font_get_texture_handle(FontAsset *asset)
{
    TextureHandle result = asset->texture_handle;

    return result;
}

static inline s32 char_index(char ch)
{
    s32 result = (s32)ch - (s32)FONT_FIRST_CHAR;
    ASSERT((result >= 0) && (result < FONT_CHAR_COUNT));

    return result;
}

static inline f32 get_text_scale(s32 text_size)
{
    // TODO: fix this calculation
    f32 result = ((f32)text_size / (f32)FONT_RASTERIZED_SIZE) * 1.6f;

    return result;
}

f32 font_get_newline_advance(FontAsset *asset, s32 text_size)
{
    (void)asset;
    // TODO: fix this
    f32 scale = get_text_scale(text_size);
    f32 result = (f32)FONT_RASTERIZED_SIZE * scale;

    return result;
}

GlyphVertices font_get_glyph_vertices(FontAsset *asset, char ch, Vector2 position, s32 font_size, RGBA32 color,
    YDirection y_dir)
{
    stbtt_packedchar char_info = asset->glyph_metrics[char_index(ch)];
    stbtt_aligned_quad quad = asset->glyph_quads[char_index(ch)];

    f32 scale = get_text_scale(font_size);

    // TODO: fix glyph offsets when y dir is down

    Vector2 glyph_size = {
        (quad.x1 - quad.x0) * scale,
        (quad.y1 - quad.y0) * scale,
    };

#if 1
    Vector2 glyph_bottom_left = {
        position.x + char_info.xoff * scale,
        position.y - char_info.yoff2 * scale
    };
#else

    f32 s = font_get_newline_advance(asset, font_size) * 0.5f;
    Vector2 glyph_bottom_left = {
        position.x + char_info.xoff * scale,
        position.y + char_info.yoff * scale + s
    };
#endif

    Vertex tl = {
        v2_add(glyph_bottom_left, v2(0.0f, glyph_size.y)),
        {quad.s0, (y_dir == Y_IS_UP) ? quad.t0 : quad.t1},
        color
    };

    Vertex tr = {
        v2_add(glyph_bottom_left, glyph_size),
        {quad.s1, (y_dir == Y_IS_UP) ? quad.t0 : quad.t1},
        color
    };

    Vertex br = {
        v2_add(glyph_bottom_left, v2(glyph_size.x, 0.0f)),
        {quad.s1, (y_dir == Y_IS_UP) ? quad.t1 : quad.t0},
        color
    };

    Vertex bl = {
        glyph_bottom_left,
        {quad.s0, (y_dir == Y_IS_UP) ? quad.t1 : quad.t0},
        color
    };

    f32 advance_x = char_info.xadvance * scale;

    GlyphVertices result = {
        .top_left = tl,
        .top_right = tr,
        .bottom_right = br,
        .bottom_left = bl,
        .advance_x = advance_x
    };

    return result;
}

Vector2 font_get_text_dimensions(FontAsset *asset, String text, s32 text_size)
{
    s32 ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&asset->font_info, &ascent, &descent, &line_gap);

    // TODO: calculate newline gap this way to
    f32 font_height = (f32)(line_gap + ascent - descent);

    Vector2 result = {0.0f, font_height};

    f32 max_x = 0.0f;
    for (s32 i = 0; i < text.length; ++i) {
        char ch = text.data[i];

        if (ch != '\n') {
            stbtt_packedchar char_info = asset->glyph_metrics[char_index(ch)];

            result.x += char_info.xadvance;
            max_x = MAX(result.x, max_x);
        } else {
            max_x = MAX(result.x, max_x);

            result.x = 0.0f;
            result.y += font_height;
        }
    }

    f32 width_scale = get_text_scale(text_size);
    f32 height_scale = stbtt_ScaleForPixelHeight(&asset->font_info, (f32)text_size);

    result.x *= width_scale;
    result.y *= height_scale;

    return result;
}
