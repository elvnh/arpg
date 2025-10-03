#include "asset_manager.h"
#include "asset.h"
#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/utils.h"
#include "font.h"
#include "renderer/renderer_backend.h"
#include "base/image.h"
#include "platform.h"

#define ASSET_ARENA_SIZE      MB(8)
#define FIRST_VALID_ASSET_ID (NULL_ASSET_ID + 1)

typedef struct {
    AssetID    id;
    AssetSlot *slot;
} AssetSlotWithID;

static AssetSlot *get_asset_slot(AssetManager *mgr, AssetID id)
{
    ASSERT(id >= FIRST_VALID_ASSET_ID);
    ASSERT(id < MAX_REGISTERED_ASSETS);

    AssetSlot *result = &mgr->registered_assets[id - FIRST_VALID_ASSET_ID];

    return result;
}

static AssetSlotWithID allocate_asset_slot(AssetManager *assets, String asset_path)
{
    AssetID id = assets->next_asset_id++;
    ASSERT(id < MAX_REGISTERED_ASSETS);

    AssetSlot *slot = get_asset_slot(assets, id);

    if (asset_path.data) {
        slot->absolute_asset_path = str_copy(asset_path, fl_allocator(&assets->asset_arena));
    }

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

static void *get_asset_data(AssetManager *assets, AssetID id, AssetKind kind)
{
    ASSERT(id < assets->next_asset_id);

    AssetSlot *asset = get_asset_slot(assets, id);
    ASSERT(asset->kind == kind);

    switch (kind) {
	case ASSET_KIND_SHADER:  return asset->as.shader_asset;
	case ASSET_KIND_TEXTURE: return asset->as.texture_asset;
	case ASSET_KIND_FONT:    return asset->as.font_asset;
    }

    ASSERT(false);

    return 0;
}

void assets_initialize(AssetManager *asset_mgr, Allocator parent_allocator)
{
    asset_mgr->asset_arena = fl_create(parent_allocator, ASSET_ARENA_SIZE);
    asset_mgr->next_asset_id = FIRST_VALID_ASSET_ID;
}

AssetSlot *assets_get_asset_by_path(AssetManager *assets, String path, LinearArena *scratch)
{
    // TODO: if number of assets grow large, linear search could become slow, but should be fine for now
    String abs_path = platform_get_canonical_path(path, la_allocator(scratch), scratch);

    for (ssize i = 0; i < assets->next_asset_id; ++i) {
        AssetSlot *slot = &assets->registered_assets[i];

        if (str_equal(slot->absolute_asset_path, abs_path)) {
            return slot;
        }
    }

    return 0;
}

static String get_absolute_asset_path(String name, AssetKind kind, LinearArena *scratch_arena)
{
    Allocator scratch = la_allocator(scratch_arena);

    String result = str_concat(platform_get_executable_directory(scratch, scratch_arena), str_lit("/"), scratch);

    switch (kind) {
        case ASSET_KIND_SHADER: {
            result = str_concat(result, str_lit(SHADER_DIRECTORY), scratch);
        } break;

        case ASSET_KIND_TEXTURE: {
            result = str_concat(result, str_lit(SPRITE_DIRECTORY), scratch);
        } break;

        case ASSET_KIND_FONT: {
            result = str_concat(result, str_lit(FONT_DIRECTORY), scratch);
        } break;

        INVALID_DEFAULT_CASE;
    }

    result = str_concat(result, name, scratch);

    return result;
}

static ShaderAsset *load_asset_data_shader(AssetManager *assets, String path, LinearArena *scratch)
{
    ShaderAsset *result = 0;
    String shader_source = platform_read_entire_file_as_string(path, la_allocator(scratch), scratch);

    if (shader_source.data) {
        ShaderAsset *shader = renderer_backend_create_shader(shader_source, fl_allocator(&assets->asset_arena));

        result = shader;
    }

    return result;
}

ShaderHandle assets_register_shader(AssetManager *assets, String name, LinearArena *scratch)
{
    String path = get_absolute_asset_path(name, ASSET_KIND_SHADER, scratch);

    AssetSlotWithID slot_and_id = allocate_asset_slot(assets, path);
    ShaderAsset *shader = load_asset_data_shader(assets, path, scratch);
    ASSERT(shader);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_SHADER, shader);

    return (ShaderHandle){slot_and_id.id};
}

static TextureAsset *load_asset_data_texture(AssetManager *assets, String path, LinearArena *scratch)
{
    TextureAsset *result = 0;
    Span file_contents = platform_read_entire_file(path, la_allocator(scratch), scratch);

    if (file_contents.data && file_contents.size) {
        Image image = image_decode_png(file_contents, la_allocator(scratch));

        if (image.data) {
            TextureAsset *texture = renderer_backend_create_texture(image, fl_allocator(&assets->asset_arena));

            result = texture;
        }
    }

    return result;
}

TextureHandle assets_register_texture(AssetManager *assets, String name, LinearArena *scratch)
{
    String path = get_absolute_asset_path(name, ASSET_KIND_TEXTURE, scratch);

    AssetSlotWithID slot_and_id = allocate_asset_slot(assets, path);
    TextureAsset *texture = load_asset_data_texture(assets, path, scratch);
    ASSERT(texture);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_TEXTURE, texture);

    return (TextureHandle){slot_and_id.id};
}

static FontAsset *load_asset_data_font(AssetManager *assets, String path, LinearArena *scratch)
{
    FontAsset *result = font_create_atlas(path, assets, fl_allocator(&assets->asset_arena), scratch);

    return result;
}

FontHandle assets_register_font(AssetManager *assets, String name, LinearArena *scratch)
{
    // TODO: reduce code duplication in assets_register functions
    String path = get_absolute_asset_path(name, ASSET_KIND_FONT, scratch);

    AssetSlotWithID slot_and_id = allocate_asset_slot(assets, path);
    FontAsset *font = load_asset_data_font(assets, path, scratch);
    ASSERT(font);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_FONT, font);

    FontHandle result = {slot_and_id.id};
    return result;
}

ShaderAsset *assets_get_shader(AssetManager *assets, ShaderHandle handle)
{
    ShaderAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_SHADER);

    return result;
}

TextureAsset *assets_get_texture(AssetManager *assets, TextureHandle handle)
{
    TextureAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_TEXTURE);

    return result;
}

FontAsset *assets_get_font(AssetManager *assets, FontHandle handle)
{
    FontAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_FONT);

    return result;
}

static b32 assets_reload_shader(AssetManager *assets, AssetSlot *slot, LinearArena *scratch)
{
    ShaderAsset *new_shader = load_asset_data_shader(assets, slot->absolute_asset_path, scratch);

    if (new_shader) {
        renderer_backend_destroy_shader(slot->as.shader_asset, fl_allocator(&assets->asset_arena));
        assign_asset_slot_data(slot, ASSET_KIND_SHADER, new_shader);

        return true;
    }

    return false;
}

static b32 assets_reload_texture(AssetManager *assets, AssetSlot *slot, LinearArena *scratch)
{
    TextureAsset *new_texture = load_asset_data_texture(assets, slot->absolute_asset_path, scratch);

    if (new_texture) {
        renderer_backend_destroy_texture(slot->as.texture_asset, fl_allocator(&assets->asset_arena));
        assign_asset_slot_data(slot, ASSET_KIND_TEXTURE, new_texture);

        return true;
    }

    return false;
}

static b32 assets_reload_font(AssetManager *assets, AssetSlot *slot, LinearArena *scratch)
{
    FontAsset *new_font = load_asset_data_font(assets, slot->absolute_asset_path, scratch);

    if (new_font) {
	font_destroy_atlas(slot->as.font_asset, fl_allocator(&assets->asset_arena));
	assign_asset_slot_data(slot, ASSET_KIND_FONT, new_font);
    }

    return false;
}

b32 assets_reload_asset(AssetManager *assets, AssetSlot *slot, LinearArena *scratch)
{
    switch (slot->kind) {
        case ASSET_KIND_SHADER: return assets_reload_shader(assets, slot, scratch);
        case ASSET_KIND_TEXTURE: return assets_reload_texture(assets, slot, scratch);
        case ASSET_KIND_FONT: return assets_reload_font(assets, slot, scratch);

        INVALID_DEFAULT_CASE;
    }

    return false;
}

TextureHandle assets_create_texture_from_memory(AssetManager *assets, Image image)
{
    // TODO: ensure that this doesn't get unloaded since it's not tied to a path and
    // can't be reloaded
    TextureAsset *texture = renderer_backend_create_texture(image, fl_allocator(&assets->asset_arena));
    AssetSlotWithID slot_and_id = allocate_asset_slot(assets, null_string);
    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_TEXTURE, texture);

    TextureHandle result = {slot_and_id.id};
    return result;
}
