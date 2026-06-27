#ifndef COMPILED_ASSET_H
#define COMPILED_ASSET_H

#include "binary_asset.h"

typedef struct CompiledAsset {
    String name;
    String path;

    BinaryAsset *asset;

    struct CompiledAsset *next;
} CompiledAsset;

typedef struct {
    CompiledAsset *head;
    CompiledAsset *tail;
} CompiledAssetList;

static inline ssize binary_texture_size(BinaryAsset *asset)
{
    ASSERT(asset->kind == ASSET_TEXTURE);

    BinaryTexture *texture = (BinaryTexture *)asset;
    ssize result = texture->width * texture->height * 4;

    return result;
}

#endif // COMPILED_ASSET_H
