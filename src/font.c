#include <stb_truetype.h>

#include "base/linear_arena.h"
#include "base/matrix.h"
#include "base/rectangle.h"
#include "base/utils.h"
#include "font.h"
#include "asset_manager.h"
#include "platform.h"
#include "renderer/renderer_backend.h"

// TODO: dynamically decide size of atlas
#define FONT_ATLAS_WIDTH             2048
#define FONT_ATLAS_HEIGHT            2048
#define FONT_FIRST_CHAR              ' '
#define FONT_LAST_CHAR               '~'
#define FONT_CHAR_COUNT              (FONT_LAST_CHAR - FONT_FIRST_CHAR)
#define FONT_RASTERIZED_SIZE         128

typedef struct {
    /* NOTE: These are unscaled */
    f32 advance_x;
    f32 left_side_bearing;

    Vector2 uv_top_left;
    Vector2 uv_top_right;
    Vector2 uv_bottom_right;
    Vector2 uv_bottom_left;
} GlyphInfo;

struct FontAsset {
    Span                 font_file_buffer; // NOTE: Must stay loaded
    TextureHandle        texture_handle;
    stbtt_fontinfo       font_info;
    GlyphInfo            glyph_infos[FONT_CHAR_COUNT];

    /* NOTE: These are unscaled */
    f32                  ascent;
    f32                  descent;
    f32                  line_gap;
};

static inline s32 char_index(char ch)
{
    s32 result = (s32)ch - (s32)FONT_FIRST_CHAR;
    ASSERT((result >= 0) && (result < FONT_CHAR_COUNT));

    return result;
}

// TODO: Most calculations in this file are pretty hacky, fix them
FontAsset *font_create_atlas(String font_path, struct AssetManager *assets, Allocator allocator,
    LinearArena *scratch)
{
    FontAsset *result = allocate_item(allocator, FontAsset);
    Span font_file_contents = platform_read_entire_file(font_path, allocator, scratch);

    if (!font_file_contents.data) {
        goto error;
    }

    result->font_file_buffer = font_file_contents;

    if (!stbtt_InitFont(&result->font_info, font_file_contents.data, 0)) {
        goto error;
    }

    byte *mono_font_bitmap = la_allocate_array(scratch, byte, FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT);

    stbtt_pack_context pack_ctx;
    if (!stbtt_PackBegin(&pack_ctx, mono_font_bitmap, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, 1, 0)) {
        goto error;
    }

    stbtt_PackSetOversampling(&pack_ctx, 2, 2);

    stbtt_packedchar *glyph_metrics = la_allocate_array(scratch, stbtt_packedchar, FONT_CHAR_COUNT);
    if (!stbtt_PackFontRange(&pack_ctx, font_file_contents.data, 0, FONT_RASTERIZED_SIZE,
            FONT_FIRST_CHAR, FONT_CHAR_COUNT, glyph_metrics)) {
        goto error;
    }

    stbtt_PackEnd(&pack_ctx);

    f32 unused_x, unused_y;
    for (s32 i = 0; i < FONT_CHAR_COUNT; ++i) {
        stbtt_aligned_quad quad;
        stbtt_GetPackedQuad(glyph_metrics, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT,
            i, &unused_x, &unused_y, &quad, 0);

        char ch = (char)(FONT_FIRST_CHAR + i);
        s32 advance_x, lsb;
        stbtt_GetCodepointHMetrics(&result->font_info, ch, &advance_x, &lsb);

        result->glyph_infos[i] = (GlyphInfo) {
            .advance_x = (f32)advance_x,
            .left_side_bearing = (f32)lsb,
            .uv_top_left = {quad.s0, quad.t0},
            .uv_top_right = {quad.s1, quad.t0},
            .uv_bottom_right = {quad.s1, quad.t1},
            .uv_bottom_left = {quad.s0, quad.t1}
        };
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

    s32 ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&result->font_info, &ascent, &descent, &line_gap);

    result->ascent = (f32)ascent;
    result->descent = (f32)descent;
    result->line_gap = (f32)line_gap;
    result->texture_handle = assets_create_texture_from_memory(assets, font_image);

    return result;

  error:
    ASSERT(0);
    font_destroy_atlas(result, allocator);

    return 0;
}

void font_destroy_atlas(FontAsset *asset, Allocator allocator)
{
    ASSERT(asset);

    if (asset->font_file_buffer.data) {
        deallocate(allocator, asset->font_file_buffer.data);
    }

    deallocate(allocator, asset);
}

TextureHandle font_get_texture_handle(FontAsset *asset)
{
    TextureHandle result = asset->texture_handle;

    return result;
}


static inline f32 get_text_scale(FontAsset *asset, s32 text_size)
{
    f32 result = stbtt_ScaleForPixelHeight(&asset->font_info, (f32)text_size);

    return result;
}

f32 font_get_baseline_offset(FontAsset *asset, s32 text_size)
{
    f32 scale = get_text_scale(asset, text_size);
    f32 result = asset->ascent * scale;

    return result;
}

f32 font_get_newline_advance(FontAsset *asset, s32 text_size)
{
    f32 scale = get_text_scale(asset, text_size);

    f32 result = (asset->ascent - asset->descent + asset->line_gap) * scale;

    return result;
}

RenderedGlyphInfo font_get_clipped_glyph_vertices(FontAsset *asset, char ch, Vector2 position,
    Rectangle bounds, s32 text_size, RGBA32 color, YDirection y_dir)
{
    GlyphInfo glyph_info = asset->glyph_infos[char_index(ch)];

    f32 scale = get_text_scale(asset, text_size);

    f32 advance_x = roundf(glyph_info.advance_x * scale);
    f32 lsb = roundf(glyph_info.left_side_bearing * scale);

    s32 x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&asset->font_info, ch, scale, scale, &x0, &y0, &x1, &y1);

    Vector2 glyph_size = {(f32)(x1 - x0), (f32)(y1 - y0)};
    Vector2 glyph_bottom_left;

    if (y_dir == Y_IS_DOWN) {
        glyph_bottom_left = v2(position.x + lsb, position.y + (f32)y0);
    } else {
        glyph_bottom_left = v2(position.x + lsb, position.y - (f32)y1);
    }

    Rectangle glyph_rect = {glyph_bottom_left, glyph_size};

    f32 uv_left = glyph_info.uv_top_left.x;
    f32 uv_right = glyph_info.uv_top_right.x;
    f32 uv_top = (y_dir == Y_IS_UP) ? glyph_info.uv_top_left.y : glyph_info.uv_bottom_left.y;
    f32 uv_bottom = (y_dir == Y_IS_UP) ? glyph_info.uv_bottom_left.y : glyph_info.uv_top_left.y;

    RectangleUVCoords uv_coords = {
	.top_left = {uv_left, uv_top},
	.top_right = {uv_right, uv_top},
	.bottom_right = {uv_right, uv_bottom},
	.bottom_left = {uv_left, uv_bottom},
    };

    ClippedRectangleVertices clipped_vertices = {0};
    if (ch != ' ') {
        clipped_vertices = rect_get_clipped_vertices_with_uvs(glyph_rect, bounds, color, y_dir, uv_coords);
    }

    RenderedGlyphInfo result = {
	.vertices = clipped_vertices.vertices,
	.advance_x = advance_x,
	.is_visible = clipped_vertices.is_visible
    };

    return result;
}

RenderedGlyphInfo font_get_glyph_vertices(FontAsset *asset, char ch, Vector2 position, s32 text_size, RGBA32 color, YDirection y_dir)
{
    // TODO: don't set this arbitrary value, use the window bounds
    Rectangle glyph_bounds = {{0, 0}, {100000, 100000}};

    RenderedGlyphInfo result = font_get_clipped_glyph_vertices(
	asset, ch, position, glyph_bounds, text_size, color, y_dir);

    ASSERT(result.is_visible || ch == ' ');

    return result;
}

Vector2 font_get_text_dimensions(FontAsset *asset, String text, s32 text_size)
{
    f32 scale = get_text_scale(asset, text_size);
    f32 font_height = asset->ascent - asset->descent;

    Vector2 result = {0.0f, font_height};

    f32 max_x = 0.0f;
    for (s32 i = 0; i < text.length; ++i) {
        char ch = text.data[i];

        if (ch == '\n') {
            max_x = MAX(result.x, max_x);

            result.x = 0.0f;
            result.y += font_height;
        } else if (ch == '\t') {
            GlyphInfo glyph_info = asset->glyph_infos[char_index(' ')];

            result.x += (glyph_info.advance_x + glyph_info.left_side_bearing) * 4;
            max_x = MAX(result.x, max_x);
        } else {
            GlyphInfo glyph_info = asset->glyph_infos[char_index(ch)];

            result.x += glyph_info.advance_x + glyph_info.left_side_bearing;
            max_x = MAX(result.x, max_x);
        }
    }


    result.x *= scale;
    result.y *= scale;

    ASSERT(result.x > 0.0f || (text.length == 0));
    ASSERT(result.y > 0.0f || (text.length == 0));

    return result;
}
