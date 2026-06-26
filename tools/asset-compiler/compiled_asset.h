#ifndef COMPILED_ASSET_H
#define COMPILED_ASSET_H

typedef enum {
    ASSET_TEXTURE,
} AssetKind2;

typedef struct {
    AssetKind2 kind;
    u32 asset_id;
} AssetHeader;

// TODO: change "Serialized"/"Compiled" prefix to "Binary"

typedef struct {
    AssetHeader header;

    s32 width;
    s32 height;
    byte bitmap[];
} SerializedTexture;

typedef struct CompiledAsset {
    String name;
    String path;

    AssetHeader *asset;

    struct CompiledAsset *next;
} CompiledAsset;

typedef struct {
    CompiledAsset *head;
    CompiledAsset *tail;
} CompiledAssetList;

#endif // COMPILED_ASSET_H
