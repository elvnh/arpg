#include "asset_system.h"

#include "asset.h"
#include "asset_table.h"
#include "base/image.h"
#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/temp_arena.h"
#include "base/utils.h"
#include "font.h"
#include "image_decode.h"
#include "platform/platform.h"
#include "renderer/backend/renderer_backend.h"

#define ASSET_ARENA_SIZE      MB(8)
#define MAX_REGISTERED_ASSETS 256

#define FIRST_VALID_ASSET_ID (NULL_ASSET_ID + 1)

typedef struct {
    AssetKind kind;

    union {
        ShaderAsset *shader_asset;
        TextureAsset *texture_asset;
        FontAsset *font_asset;
    } as;

    String canonical_asset_path;
} AssetSlot;

typedef struct AssetSystem {
    AssetID next_asset_id;
    AssetSlot registered_assets[MAX_REGISTERED_ASSETS];

    // TODO: use free list arena in GameMemory instead
    FreeListArena asset_arena;
} AssetSystem;

typedef struct {
    AssetID id;
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

    AssetSlotWithID result = {.id = id, .slot = slot};

    return result;
}

// TODO: are these needed or can we just memcpy into union? since C doesn't have concept of active union member?
static void assign_asset_slot_data(AssetSlot *slot, AssetKind kind, void *data)
{
    slot->kind = kind;

    BEGIN_EXHAUSTIVE_SWITCH;
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
    END_EXHAUSTIVE_SWITCH;
}

static void *get_asset_data(AssetID id, AssetKind kind)
{
    ASSERT(id < g_asset_system.next_asset_id);

    AssetSlot *asset = get_asset_slot(id);
    ASSERT(asset->kind == kind);

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (kind) {
        case ASSET_KIND_SHADER:
            return asset->as.shader_asset;
        case ASSET_KIND_TEXTURE:
            return asset->as.texture_asset;
        case ASSET_KIND_FONT:
            return asset->as.font_asset;
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    return 0;
}

void assets_initialize(Allocator parent_allocator)
{
    g_asset_system.asset_arena = fl_create(parent_allocator, ASSET_ARENA_SIZE);
    g_asset_system.next_asset_id = FIRST_VALID_ASSET_ID;
}

static AssetSlot *get_asset_by_path(String path)
{
    LinearArena scratch = temp_arena_begin();

    // NOTE: if number of assets grow large, linear search could become slow, but should be fine for now
    String abs_path = platform_get_canonical_path(path, la_allocator(&scratch));

    AssetSlot *slot = 0;
    for (ssize i = 0; i < g_asset_system.next_asset_id; ++i) {
        slot = &g_asset_system.registered_assets[i];

        if (str_equal(slot->canonical_asset_path, abs_path)) {
            break;
        }
    }

    temp_arena_end(&scratch);

    return slot;
}

static String get_canonical_asset_path(String name, AssetKind kind, FreeListArena *arena)
{
    LinearArena scratch = temp_arena_begin();

    Allocator scratch_allocator = la_allocator(&scratch);

    // NOTE: path is copied later which is why we allocate in scratch arena while building string
    // TODO: replace with format
    String path = str_concat(platform_get_executable_directory(scratch_allocator),
        str("/../" ASSET_DIRECTORY), scratch_allocator);

    path = platform_get_canonical_path(path, scratch_allocator);

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (kind) {
        case ASSET_KIND_SHADER: {
            path = str_concat(path, str("/" SHADER_DIRECTORY), scratch_allocator);
        } break;

        case ASSET_KIND_TEXTURE: {
            path = str_concat(path, str("/" SPRITE_DIRECTORY), scratch_allocator);
        } break;

        case ASSET_KIND_FONT: {
            path = str_concat(path, str("/" FONT_DIRECTORY), scratch_allocator);
        } break;

            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    path = str_concat(path, name, scratch_allocator);

    String result = str_copy(path, fl_allocator(arena));

    temp_arena_end(&scratch);

    return result;
}

static ShaderAsset *load_asset_data_shader(String path)
{
    LinearArena scratch = temp_arena_begin();

    ShaderAsset *result = 0;
    String shader_source = platform_read_entire_file_as_string(path, la_allocator(&scratch));

    if (shader_source.data) {
        ShaderAsset *shader = renderer_backend_create_shader(shader_source,
            fl_allocator(&g_asset_system.asset_arena));

        result = shader;
    }

    temp_arena_end(&scratch);

    return result;
}

ShaderHandle assets_register_shader(String name)
{
    String path =
        get_canonical_asset_path(name, ASSET_KIND_SHADER, &g_asset_system.asset_arena);

    AssetSlotWithID slot_and_id = allocate_asset_slot(path);
    ShaderAsset *shader = load_asset_data_shader(path);
    ASSERT(shader);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_SHADER, shader);

    return (ShaderHandle){slot_and_id.id};
}

static TextureAsset *load_asset_data_texture(String path)
{
    LinearArena scratch = temp_arena_begin();

    TextureAsset *result = 0;
    Span file_contents = platform_read_entire_file(path, la_allocator(&scratch));

    if (file_contents.data && file_contents.size) {
        Image image = image_decode_png(file_contents, la_allocator(&scratch));

        if (image.data) {
            TextureAsset *texture = renderer_backend_create_texture(image,
                fl_allocator(&g_asset_system.asset_arena));

            result = texture;
        }
    }

    temp_arena_end(&scratch);

    return result;
}

TextureHandle assets_register_texture(String name)
{
    String path =
        get_canonical_asset_path(name, ASSET_KIND_TEXTURE, &g_asset_system.asset_arena);

    AssetSlotWithID slot_and_id = allocate_asset_slot(path);
    TextureAsset *texture = load_asset_data_texture(path);
    ASSERT(texture);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_TEXTURE, texture);

    return (TextureHandle){slot_and_id.id};
}

static FontAsset *load_asset_data_font(String path)
{
    FontAsset *result = font_create_atlas(path, fl_allocator(&g_asset_system.asset_arena));

    return result;
}

FontHandle assets_register_font(String name)
{
    // TODO: reduce code duplication in assets_register functions
    String path = get_canonical_asset_path(name, ASSET_KIND_FONT, &g_asset_system.asset_arena);

    AssetSlotWithID slot_and_id = allocate_asset_slot(path);
    FontAsset *font = load_asset_data_font(path);
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

static b32 assets_reload_shader(AssetSlot *slot)
{
    ShaderAsset *new_shader = load_asset_data_shader(slot->canonical_asset_path);

    if (new_shader) {
        renderer_backend_destroy_shader(slot->as.shader_asset,
            fl_allocator(&g_asset_system.asset_arena));
        assign_asset_slot_data(slot, ASSET_KIND_SHADER, new_shader);

        return true;
    }

    return false;
}

static b32 assets_reload_texture(AssetSlot *slot)
{
    TextureAsset *new_texture = load_asset_data_texture(slot->canonical_asset_path);

    if (new_texture) {
        renderer_backend_destroy_texture(slot->as.texture_asset,
            fl_allocator(&g_asset_system.asset_arena));
        assign_asset_slot_data(slot, ASSET_KIND_TEXTURE, new_texture);

        return true;
    }

    return false;
}

static b32 assets_reload_font(AssetSlot *slot)
{
    FontAsset *new_font = load_asset_data_font(slot->canonical_asset_path);

    if (new_font) {
        font_destroy_atlas(slot->as.font_asset, fl_allocator(&g_asset_system.asset_arena));
        assign_asset_slot_data(slot, ASSET_KIND_FONT, new_font);
    }

    return false;
}

b32 assets_reload_asset_with_path(String path)
{
    AssetSlot *slot = get_asset_by_path(path);

    if (slot) {
        BEGIN_EXHAUSTIVE_SWITCH;
        switch (slot->kind) {
            case ASSET_KIND_SHADER:
                return assets_reload_shader(slot);
            case ASSET_KIND_TEXTURE:
                return assets_reload_texture(slot);
            case ASSET_KIND_FONT:
                return assets_reload_font(slot);

                INVALID_DEFAULT_CASE;
        }
        END_EXHAUSTIVE_SWITCH;
    }

    return false;
}

TextureHandle assets_create_texture_from_memory(Image image)
{
    // TODO: ensure that this doesn't get unloaded since it's not tied to a path and
    // can't be reloaded
    TextureAsset *texture =
        renderer_backend_create_texture(image, fl_allocator(&g_asset_system.asset_arena));
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
AssetTable load_game_assets(void)
{
    AssetTable result = {0};

#define DEFINE_ASSET(name, type, path)                                                   \
    switch (get_game_asset_kind(ASSET_NAME_TO_ENUM(name))) {                             \
        case ASSET_KIND_TEXTURE: {                                                       \
            result.textures[ASSET_NAME_TO_ENUM(name)] =                                  \
                assets_register_texture(str(path));                             \
        } break;                                                                         \
        case ASSET_KIND_SHADER: {                                                        \
            result.shaders[ASSET_NAME_TO_ENUM(name)] =                                   \
                assets_register_shader(str(path));                              \
        } break;                                                                         \
        case ASSET_KIND_FONT: {                                                          \
            result.fonts[ASSET_NAME_TO_ENUM(name)] =                                     \
                assets_register_font(str(path));                                \
        } break;                                                                         \
    }

    GAME_ASSET_LIST

#undef DEFINE_ASSET

    return result;
}
