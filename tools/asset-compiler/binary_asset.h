#ifndef BINARY_ASSET_H
#define BINARY_ASSET_H

typedef enum {
    ASSET_TEXTURE,
} AssetKind2; // TODO: rename

typedef struct {
    AssetKind2 kind;
    u32 asset_id;
} BinaryAsset;

typedef struct {
    BinaryAsset header;

    s32 width;
    s32 height;
    byte bitmap[];
} BinaryTexture;

#endif // BINARY_ASSET_H
