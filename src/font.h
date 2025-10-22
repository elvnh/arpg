 #ifndef FONT_H
#define FONT_H

#include "base/allocator.h"
#include "base/span.h"
#include "base/vector.h"
#include "base/matrix.h"
#include "base/linear_arena.h"
#include "base/vertex.h"
#include "base/string8.h"
#include "base/rectangle.h"
#include "asset.h"

typedef struct FontAsset FontAsset;
struct AssetManager;

/*
  TODO:
  - Redo size calculations properly
  - font_info unneeded?
  - Destroying font asset
  - Different colors in same string
  - Font hot reloading
  - store font asset instead of handle?
  - Include stb_rect_pack?
  - Clean up return types for getting vertices
 */

// TODO: rename
typedef struct {
    RectangleVertices vertices;
    f32 advance_x;
    b32 is_visible;
} RenderedGlyphInfo;

FontAsset     *font_create_atlas(String font_path, struct AssetManager *assets, Allocator allocator, LinearArena *scratch);
void           font_destroy_atlas(FontAsset *asset, Allocator allocator);
TextureHandle  font_get_texture_handle(FontAsset *asset);
RenderedGlyphInfo  font_get_glyph_vertices(FontAsset *asset, char ch, Vector2 position, s32 font_size,
    RGBA32 color, YDirection y_dir);
RenderedGlyphInfo font_get_clipped_glyph_vertices(FontAsset *asset, char ch, Vector2 position,
    Rectangle bounds, s32 text_size, RGBA32 color, YDirection y_dir);
f32            font_get_newline_advance(FontAsset *asset, s32 text_size);
Vector2        font_get_text_dimensions(FontAsset *asset, String text, s32 text_size);
f32            font_get_baseline_offset(FontAsset *asset, s32 text_size);



#endif //FONT_H
