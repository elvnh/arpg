#pragma once
// TODO: use normal header guard

#include "asset2.h"
#include "base/free_list_arena.h"
#include "generated/asset_handle.h"

typedef struct {
    TextureAsset2 *textures[TEXTURE_ASSET_COUNT];

    FreeListArena arena;
} AssetManager;

b32 asset_mgr_initialize(AssetManager *mgr, Allocator allocator);
