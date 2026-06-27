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

#endif // COMPILED_ASSET_H
