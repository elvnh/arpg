#ifndef ASSET_TABLE_H
#define ASSET_TABLE_H

#include "base/utils.h"
#include "platform/asset.h"

// TODO: capitalize names
// TODO: separate enums for different asset types

#define GAME_ASSET_LIST                                                 \
    DEFINE_ASSET(TEXTURE_SHADER, SHADER, "shader.glsl")                 \
    DEFINE_ASSET(SHAPE_SHADER, SHADER, "shader2.glsl")                  \
    DEFINE_ASSET(LIGHT_SHADER, SHADER, "light.glsl")                    \
    DEFINE_ASSET(LIGHT_BLENDING_SHADER, SHADER, "light_blending.glsl")  \
    DEFINE_ASSET(SCREENSPACE_TEXTURE_SHADER, SHADER, "screenspace_texture.glsl") \
    DEFINE_ASSET(DEFAULT_TEXTURE, TEXTURE, "test.png")                  \
    DEFINE_ASSET(FIREBALL_TEXTURE, TEXTURE, "fireball.png")             \
    DEFINE_ASSET(SPARK_TEXTURE, TEXTURE, "spark.png")                   \
    DEFINE_ASSET(BLIZZARD_TEXTURE, TEXTURE, "blizzard.png")             \
    DEFINE_ASSET(PLAYER_IDLE1, TEXTURE, "player_idle1.png")             \
    DEFINE_ASSET(PLAYER_IDLE2, TEXTURE, "player_idle2.png")             \
    DEFINE_ASSET(PLAYER_WALKING1, TEXTURE, "player_walking1.png")       \
    DEFINE_ASSET(PLAYER_WALKING2, TEXTURE, "player_walking2.png")       \
    DEFINE_ASSET(PLAYER_ATTACK1, TEXTURE, "player_attack1.png")         \
    DEFINE_ASSET(PLAYER_ATTACK2, TEXTURE, "player_attack2.png")         \
    DEFINE_ASSET(PLAYER_ATTACK3, TEXTURE, "player_attack3.png")         \
    DEFINE_ASSET(FLOOR_TEXTURE, TEXTURE, "floor.png")                   \
    DEFINE_ASSET(WALL_TEXTURE, TEXTURE, "wall.png")                     \
    DEFINE_ASSET(ICE_SHARD_TEXTURE, TEXTURE, "ice_shard.png")           \
    DEFINE_ASSET(DEFAULT_FONT, FONT, "Ubuntu-M.ttf")                    \

#define ASSET_NAME_TO_ENUM(name) GAME_ASSET_##name

#define texture_handle(name) texture_handle_impl(ASSET_NAME_TO_ENUM(name))
#define shader_handle(name)  shader_handle_impl(ASSET_NAME_TO_ENUM(name))
#define font_handle(name)    font_handle_impl(ASSET_NAME_TO_ENUM(name))

typedef enum {
#define DEFINE_ASSET(name, type, path) ASSET_NAME_TO_ENUM(name),
    GAME_ASSET_LIST
#undef DEFINE_ASSET

    GAME_ASSET_COUNT
} GameAsset;

typedef struct {
    // TODO: only make the arrays as large as needed
    ShaderHandle   shaders[GAME_ASSET_COUNT];
    TextureHandle  textures[GAME_ASSET_COUNT];
    FontHandle     fonts[GAME_ASSET_COUNT];
} AssetTable;

void          set_global_asset_table(AssetTable *assets);
TextureHandle texture_handle_impl(GameAsset asset);
ShaderHandle  shader_handle_impl(GameAsset asset);
FontHandle    font_handle_impl(GameAsset asset);

static inline AssetKind get_game_asset_kind(GameAsset asset_name)
{
    switch (asset_name) {
#define DEFINE_ASSET(name, type, path) case ASSET_NAME_TO_ENUM(name): return ASSET_KIND_##type;
        GAME_ASSET_LIST
#undef DEFINE_ASSET

       INVALID_DEFAULT_CASE;
    }

    return 0;
}

static inline TextureHandle get_texture_handle_from_table(AssetTable *assets, GameAsset asset)
{
    ASSERT(get_game_asset_kind(asset) == ASSET_KIND_TEXTURE);
    TextureHandle result = assets->textures[asset];

    return result;
}

static inline ShaderHandle get_shader_handle_from_table(AssetTable *assets, GameAsset asset)
{
    ASSERT(get_game_asset_kind(asset) == ASSET_KIND_SHADER);
    ShaderHandle result = assets->shaders[asset];

    return result;
}

static inline FontHandle get_font_handle_from_table(AssetTable *assets, GameAsset asset)
{
    ASSERT(get_game_asset_kind(asset) == ASSET_KIND_FONT);
    FontHandle result = assets->fonts[asset];

    return result;
}


#endif //ASSET_TABLE_H
