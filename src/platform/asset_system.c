#include "asset_system.h"
#include "asset.h"
#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/utils.h"
#include "font.h"
#include "renderer/backend/renderer_backend.h"
#include "base/image.h"
#include "platform/platform.h"

#define ASSET_ARENA_SIZE      MB(8)
#define FIRST_VALID_ASSET_ID (NULL_ASSET_ID + 1)


#define MAX_REGISTERED_ASSETS 256

typedef struct AssetSystem {
    AssetID       next_asset_id;
    AssetSlot     registered_assets[MAX_REGISTERED_ASSETS];

    // TODO: use free list arena in GameMemory instead
    FreeListArena asset_arena;
} AssetSystem;

typedef struct {
    AssetID    id;
    AssetSlot *slot;
} AssetSlotWithID;

static AssetSystem g_asset_system;

static AssetSlot *get_asset_slot(AssetID id)
{
    ASSERT(id >= FIRST_VALID_ASSET_ID);
    ASSERT(id < MAX_REGISTERED_ASSETS);

    AssetSlot *result = &g_asset_system.registered_assets[id - FIRST_VALID_ASSET_ID];

    return result;
}
static AssetSlotWithID allocate_asset_slot(String asset_path)

{
    AssetID id = g_asset_system.next_asset_id++;
    ASSERT(id < MAX_REGISTERED_ASSETS);

    AssetSlot *slot = get_asset_slot(id);

    slot->canonical_asset_path = asset_path;

    AssetSlotWithID result = {
        .id = id,
        .slot = slot
    };

    return result;
}

// TODO: are these needed or can we just memcpy into union? since C doesn't have concept of active union member?
static void assign_asset_slot_data(AssetSlot *slot, AssetKind kind, void *data)
{
    slot->kind = kind;

    switch (kind) {
        case ASSET_KIND_TEXTURE: {
            slot->as.texture_asset = data;
        } break;

        case ASSET_KIND_SHADER: {
            slot->as.shader_asset = data;
        } break;

        case ASSET_KIND_FONT: {
            slot->as.shader_asset = data;
        } break;

        INVALID_DEFAULT_CASE;
    }
}

static void *get_asset_data(AssetID id, AssetKind kind)
{
    ASSERT(id < g_asset_system.next_asset_id);

    AssetSlot *asset = get_asset_slot(id);
    ASSERT(asset->kind == kind);

    switch (kind) {
	case ASSET_KIND_SHADER:  return asset->as.shader_asset;
	case ASSET_KIND_TEXTURE: return asset->as.texture_asset;
	case ASSET_KIND_FONT:    return asset->as.font_asset;
    }

    ASSERT(false);

    return 0;
}

void assets_initialize(Allocator parent_allocator)
{
    g_asset_system.asset_arena = fl_create(parent_allocator, ASSET_ARENA_SIZE);
    g_asset_system.next_asset_id = FIRST_VALID_ASSET_ID;
}

AssetSlot *assets_get_asset_by_path(String path, LinearArena *scratch)
{
    // NOTE: if number of assets grow large, linear search could become slow, but should be fine for now
    String abs_path = platform_get_canonical_path(path, la_allocator(scratch), scratch);

    for (ssize i = 0; i < g_asset_system.next_asset_id; ++i) {
        AssetSlot *slot = &g_asset_system.registered_assets[i];

        if (str_equal(slot->canonical_asset_path, abs_path)) {
            return slot;
        }
    }

    return 0;
}

static String get_canonical_asset_path(String name, AssetKind kind, FreeListArena *arena, LinearArena *scratch_arena)
{
    Allocator scratch = la_allocator(scratch_arena);

    // NOTE: path is copied later which is why we allocate in scratch arena
    String path = str_concat(
	platform_get_executable_directory(scratch, scratch_arena),
	str_lit("/../"ASSET_DIRECTORY),
	scratch
    );

    path = platform_get_canonical_path(path, scratch, scratch_arena);

    switch (kind) {
        case ASSET_KIND_SHADER: {
            path = str_concat(path, str_lit("/"SHADER_DIRECTORY), scratch);
        } break;

        case ASSET_KIND_TEXTURE: {
            path = str_concat(path, str_lit("/"SPRITE_DIRECTORY), scratch);
        } break;

        case ASSET_KIND_FONT: {
            path = str_concat(path, str_lit("/"FONT_DIRECTORY), scratch);
        } break;

        INVALID_DEFAULT_CASE;
    }

    path = str_concat(path, name, scratch);

    String result = str_copy(path, fl_allocator(arena));

    return result;
}

static ShaderAsset *load_asset_data_shader(String path, LinearArena *scratch)
{
    ShaderAsset *result = 0;
    String shader_source = platform_read_entire_file_as_string(path, la_allocator(scratch), scratch);

    if (shader_source.data) {
        ShaderAsset *shader = renderer_backend_create_shader(shader_source, fl_allocator(&g_asset_system.asset_arena));

        result = shader;
    }

    return result;
}

ShaderHandle assets_register_shader(String name, LinearArena *scratch)
{
    String path = get_canonical_asset_path(name, ASSET_KIND_SHADER, &g_asset_system.asset_arena, scratch);

    AssetSlotWithID slot_and_id = allocate_asset_slot(path);
    ShaderAsset *shader = load_asset_data_shader(path, scratch);
    ASSERT(shader);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_SHADER, shader);

    return (ShaderHandle){slot_and_id.id};
}

static TextureAsset *load_asset_data_texture(String path, LinearArena *scratch)
{
    TextureAsset *result = 0;
    Span file_contents = platform_read_entire_file(path, la_allocator(scratch), scratch);

    if (file_contents.data && file_contents.size) {
        Image image = image_decode_png(file_contents, la_allocator(scratch));

        if (image.data) {
            TextureAsset *texture = renderer_backend_create_texture(image, fl_allocator(&g_asset_system.asset_arena));

            result = texture;
        }
    }

    return result;
}

TextureHandle assets_register_texture(String name, LinearArena *scratch)
{
    String path = get_canonical_asset_path(name, ASSET_KIND_TEXTURE, &g_asset_system.asset_arena, scratch);

    AssetSlotWithID slot_and_id = allocate_asset_slot(path);
    TextureAsset *texture = load_asset_data_texture(path, scratch);
    ASSERT(texture);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_TEXTURE, texture);

    return (TextureHandle){slot_and_id.id};
}

static FontAsset *load_asset_data_font(String path, LinearArena *scratch)
{
    FontAsset *result = font_create_atlas(path, fl_allocator(&g_asset_system.asset_arena), scratch);

    return result;
}

FontHandle assets_register_font(String name, LinearArena *scratch)
{
    // TODO: reduce code duplication in assets_register functions
    String path = get_canonical_asset_path(name, ASSET_KIND_FONT, &g_asset_system.asset_arena, scratch);

    AssetSlotWithID slot_and_id = allocate_asset_slot(path);
    FontAsset *font = load_asset_data_font(path, scratch);
    ASSERT(font);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_FONT, font);

    FontHandle result = {slot_and_id.id};
    return result;
}

ShaderAsset *assets_get_shader(ShaderHandle handle)
{
    ShaderAsset *result = get_asset_data(handle.id, ASSET_KIND_SHADER);

    return result;
}

TextureAsset *assets_get_texture(TextureHandle handle)
{
    TextureAsset *result = get_asset_data(handle.id, ASSET_KIND_TEXTURE);

    return result;
}

FontAsset *assets_get_font(FontHandle handle)
{
    FontAsset *result = get_asset_data(handle.id, ASSET_KIND_FONT);

    return result;
}

static b32 assets_reload_shader(AssetSlot *slot, LinearArena *scratch)
{
    ShaderAsset *new_shader = load_asset_data_shader(slot->canonical_asset_path, scratch);

    if (new_shader) {
        renderer_backend_destroy_shader(slot->as.shader_asset, fl_allocator(&g_asset_system.asset_arena));
        assign_asset_slot_data(slot, ASSET_KIND_SHADER, new_shader);

        return true;
    }

    return false;
}

static b32 assets_reload_texture(AssetSlot *slot, LinearArena *scratch)
{
    TextureAsset *new_texture = load_asset_data_texture(slot->canonical_asset_path, scratch);

    if (new_texture) {
        renderer_backend_destroy_texture(slot->as.texture_asset, fl_allocator(&g_asset_system.asset_arena));
        assign_asset_slot_data(slot, ASSET_KIND_TEXTURE, new_texture);

        return true;
    }

    return false;
}

static b32 assets_reload_font(AssetSlot *slot, LinearArena *scratch)
{
    FontAsset *new_font = load_asset_data_font(slot->canonical_asset_path, scratch);

    if (new_font) {
	font_destroy_atlas(slot->as.font_asset, fl_allocator(&g_asset_system.asset_arena));
	assign_asset_slot_data(slot, ASSET_KIND_FONT, new_font);
    }

    return false;
}

b32 assets_reload_asset(AssetSlot *slot, LinearArena *scratch)
{
    switch (slot->kind) {
        case ASSET_KIND_SHADER: return assets_reload_shader(slot, scratch);
        case ASSET_KIND_TEXTURE: return assets_reload_texture(slot, scratch);
        case ASSET_KIND_FONT: return assets_reload_font(slot, scratch);

        INVALID_DEFAULT_CASE;
    }

    return false;
}

TextureHandle assets_create_texture_from_memory(Image image)
{
    // TODO: ensure that this doesn't get unloaded since it's not tied to a path and
    // can't be reloaded
    TextureAsset *texture = renderer_backend_create_texture(image, fl_allocator(&g_asset_system.asset_arena));
    AssetSlotWithID slot_and_id = allocate_asset_slot(null_string);
    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_TEXTURE, texture);

    TextureHandle result = {slot_and_id.id};
    return result;
}

Vector2 assets_get_text_dimensions(FontHandle font_handle, String text, s32 text_size)
{
    FontAsset *asset = assets_get_font(font_handle);
    Vector2 result = font_get_text_dimensions(asset, text, text_size);

    return result;
}

f32 assets_get_text_newline_advance(FontHandle font_handle, s32 text_size)
{
    FontAsset *asset = assets_get_font(font_handle);
    f32 result = font_get_newline_advance(asset, text_size);

    return result;
}

f32 assets_get_font_baseline_offset(FontHandle font_handle, s32 text_size)
{
    FontAsset *asset = assets_get_font(font_handle);
    f32 result = font_get_baseline_offset(asset, text_size);

    return result;
}

AssetTable load_game_assets(LinearArena *scratch)
{
    AssetTable result = {0};

    result.texture_shader = assets_register_shader(str_lit("shader.glsl"), scratch);
    result.shape_shader = assets_register_shader(str_lit("shader2.glsl"), scratch);
    result.light_shader = assets_register_shader(str_lit("light.glsl"), scratch);
    result.light_blending_shader = assets_register_shader(str_lit("light_blending.glsl"), scratch);
    result.screenspace_texture_shader = assets_register_shader(str_lit("screenspace_texture.glsl"), scratch);
    result.default_texture = assets_register_texture(str_lit("test.png"), scratch);
    result.fireball_texture = assets_register_texture(str_lit("fireball.png"), scratch);
    result.spark_texture = assets_register_texture(str_lit("spark.png"), scratch);
    result.default_font = assets_register_font(str_lit("Ubuntu-M.ttf"), scratch);
    result.player_idle1 = assets_register_texture(str_lit("player_idle1.png"), scratch);
    result.player_idle2 = assets_register_texture(str_lit("player_idle2.png"), scratch);
    result.player_walking1 = assets_register_texture(str_lit("player_walking1.png"), scratch);
    result.player_walking2 = assets_register_texture(str_lit("player_walking2.png"), scratch);
    result.floor_texture = assets_register_texture(str_lit("floor.png"), scratch);
    result.wall_texture = assets_register_texture(str_lit("wall.png"), scratch);
    result.ice_shard_texture = assets_register_texture(str_lit("ice_shard.png"), scratch);
    result.player_attack1 = assets_register_texture(str_lit("player_attack1.png"), scratch);
    result.player_attack2 = assets_register_texture(str_lit("player_attack2.png"), scratch);
    result.player_attack3 = assets_register_texture(str_lit("player_attack3.png"), scratch);
    result.blizzard_texture = assets_register_texture(str_lit("blizzard.png"), scratch);

    return result;
}
