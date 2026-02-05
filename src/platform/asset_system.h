#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "asset.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/image.h"
#include "renderer/backend/renderer_backend.h"
#include "font.h"
#include "game/asset_table.h"

#define ASSET_DIRECTORY  "assets/"
#define SHADER_DIRECTORY "shaders/"
#define SPRITE_DIRECTORY "sprites/"
#define FONT_DIRECTORY   "fonts/"

void              assets_initialize(Allocator parent_allocator);
ShaderHandle      assets_register_shader(String name, LinearArena *scratch);
TextureHandle     assets_register_texture(String name, LinearArena *scratch);
FontHandle        assets_register_font(String name, LinearArena *scratch);
ShaderAsset      *assets_get_shader(ShaderHandle handle);
TextureAsset     *assets_get_texture(TextureHandle handle);
FontAsset        *assets_get_font(FontHandle handle);
b32               assets_reload_asset_with_path(String path, LinearArena *scratch);
TextureHandle     assets_create_texture_from_memory(Image image);
Vector2           assets_get_text_dimensions(FontHandle font_handle, String text, s32 text_size);
f32               assets_get_text_newline_advance(FontHandle font_handle, s32 text_size);
f32               assets_get_font_baseline_offset(FontHandle font_handle, s32 text_size);

AssetTable        load_game_assets(struct LinearArena *scratch);

#endif //ASSET_MANAGER_H
