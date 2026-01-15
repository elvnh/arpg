#ifndef ASSET_TABLE_H
#define ASSET_TABLE_H

#include "asset.h"

//TODO: organize animation frames in arrays of texture handles
typedef struct AssetTable {
    ShaderHandle texture_shader;
    ShaderHandle shape_shader;
    TextureHandle default_texture;
    TextureHandle fireball_texture;
    TextureHandle spark_texture;
    FontHandle default_font;

    TextureHandle player_idle1;
    TextureHandle player_idle2;
    TextureHandle player_walking1;
    TextureHandle player_walking2;
    TextureHandle player_attack1;
    TextureHandle player_attack2;
    TextureHandle player_attack3;

    TextureHandle floor_texture;
    TextureHandle wall_texture;

    TextureHandle ice_shard_texture;

} AssetTable;

// TODO: make this return a const pointer
AssetTable  *get_asset_table();
void         set_global_asset_table(AssetTable *assets);

#endif //ASSET_TABLE_H
