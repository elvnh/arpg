#ifndef COMPILED_ASSET_H
#define COMPILED_ASSET_H

typedef enum {
    ASSET_TEXTURE,
} AssetKind2;

typedef struct CompiledAsset {
    String name;
    String path;

    AssetKind2 kind;

    union {
        struct {
            s32 width;
            s32 height;
        } texture;
    } as;

    void *memory;
    ssize size;

    struct CompiledAsset *next;
} CompiledAsset;

typedef struct {
    CompiledAsset *head;
    CompiledAsset *tail;
} CompiledAssetList;

typedef struct {
    AssetKind2 kind;
    ssize size;
    byte data[];
} SerializedAsset;

typedef struct {
    s32 width;
    s32 height;
    byte bitmap[];
} SerializedTexture;

#endif // COMPILED_ASSET_H
