#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "asset.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/image.h"
#include "renderer/renderer_backend.h"
#include "font.h"

#define MAX_REGISTERED_ASSETS 256

#define ASSET_DIRECTORY  "assets/"
#define SHADER_DIRECTORY "shaders/"
#define SPRITE_DIRECTORY "sprites/"
#define FONT_DIRECTORY   "fonts/"

typedef enum {
    ASSET_KIND_SHADER,
    ASSET_KIND_TEXTURE,
    ASSET_KIND_FONT,
} AssetKind;

typedef struct {
    AssetKind kind;

    union {
	ShaderAsset  *shader_asset;
	TextureAsset *texture_asset;
	FontAsset    *font_asset;
    } as;

    String canonical_asset_path;
} AssetSlot;

typedef struct AssetSystem {
    AssetID       next_asset_id;
    AssetSlot     registered_assets[MAX_REGISTERED_ASSETS];

    // TODO: use free list arena in GameMemory instead
    FreeListArena asset_arena;
} AssetSystem;

void              assets_initialize(AssetSystem *asset_mgr, Allocator parent_allocator);
AssetSlot        *assets_get_asset_by_path(AssetSystem *assets, String path, LinearArena *scratch);
ShaderHandle      assets_register_shader(AssetSystem *assets, String name, LinearArena *scratch);
TextureHandle     assets_register_texture(AssetSystem *assets, String name, LinearArena *scratch);
FontHandle        assets_register_font(AssetSystem *assets, String name, LinearArena *scratch);
ShaderAsset      *assets_get_shader(AssetSystem *assets, ShaderHandle handle);
TextureAsset     *assets_get_texture(AssetSystem *assets, TextureHandle handle);
FontAsset        *assets_get_font(AssetSystem *assets, FontHandle handle);
b32               assets_reload_asset(AssetSystem *assets, AssetSlot *slot, LinearArena *scratch);
TextureHandle     assets_create_texture_from_memory(AssetSystem *assets, Image image);


#endif //ASSET_MANAGER_H
