#include "base/format.h"
#include "base/image.h"
#include "base/sl_list.h"
#include "base/string8.h"
#include "parser.h"
#include "platform/platform.h"
#include "stb_image.h"
#include "textures.h"

#include <stdio.h>

// TODO: clean up project structure
// TODO: Error when multiple assets with same name exist

typedef struct {
    b32 ok;

    String input_dir;
    String enums_path;
    String filenames_path;
    String output_dir;
} Args;

static void generate_asset_enum(CompiledAssetList assets, AssetKind2 asset_kind,
    String enum_name, String macro_name, StringBuilder *sb, LinearArena *arena)
{
    String enum_prefix = str_to_upper(macro_name, la_allocator(arena));

    str_builder_append(sb, format(arena, "#define " FMT_STR "(a) " FMT_STR "_##a",
                               FMT_STR_ARG(macro_name), FMT_STR_ARG(enum_prefix)));
    str_builder_append(sb, str("\n\n"));
    str_builder_append(sb, str("typedef enum {"));

    ssize count = 0;
    for (CompiledAsset *a = list_head(&assets); a; a = list_next(a)) {
        String s = format(arena, "\n    " FMT_STR "(" FMT_STR ") = %u,",
            FMT_STR_ARG(macro_name), FMT_STR_ARG(a->name), a->asset->asset_id);

        str_builder_append(sb, s);

        ++count;
    }

    str_builder_append(sb, format(arena, "\n} " FMT_STR ";\n\n", FMT_STR_ARG(enum_name)));

    String asset_kind_spelling = {0};

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (asset_kind) {
        case ASSET_TEXTURE: {
            asset_kind_spelling = str("TEXTURE");
        } break;

            INVALID_DEFAULT_CASE;
    }

    str_builder_append(sb, format(arena, "#define " FMT_STR "_ASSET_COUNT %ld\n",
                               FMT_STR_ARG(asset_kind_spelling), count));
}

static String generate_asset_enums(CompiledAssetList textures, LinearArena *arena,
    Allocator allocator)
{
    StringBuilder sb = str_builder_allocate(MB(8), allocator);

    str_builder_append(&sb, str("#pragma once\n\n"));

    generate_asset_enum(textures, ASSET_TEXTURE, str("TextureHandle"), str("texture_handle"),
        &sb, arena);

    return sb.buffer;
}

static String get_binary_asset_filename(CompiledAsset *asset, Allocator allocator)
{
    String result = str_concat(asset->name, str(".dat"), allocator);

    return result;
}

static String generate_binary_asset_file_list(CompiledAssetList textures, LinearArena *arena,
    Allocator allocator)
{
    StringBuilder sb = str_builder_allocate(MB(8), allocator);

    str_builder_append(&sb, str("static const char *binary_asset_files[] = {"));

    for (CompiledAsset *a = list_head(&textures); a; a = list_next(a)) {
        String filename = get_binary_asset_filename(a, allocator);

        String element = format(arena, "\n    \"" FMT_STR "\",", FMT_STR_ARG(filename));
        str_builder_append(&sb, element);
    }

    str_builder_append(&sb, str("\n}"));

    return sb.buffer;
}

static b32 write_binary_asset_files(CompiledAssetList assets, Args args, Allocator allocator)
{
    b32 result = false;

    for (CompiledAsset *a = list_head(&assets); a; a = list_next(a)) {
        BinaryAsset *bin_asset = a->asset;

        ssize asset_size_excluding_header = 0;

        BEGIN_EXHAUSTIVE_SWITCH;
        switch (bin_asset->kind) {
            case ASSET_TEXTURE: {
                asset_size_excluding_header = binary_texture_size(bin_asset);
            } break;

                INVALID_DEFAULT_CASE;
        }
        END_EXHAUSTIVE_SWITCH;

        ASSERT(asset_size_excluding_header > 0);

        String filename = get_binary_asset_filename(a, allocator);
        String path = platform_make_relative_to(args.output_dir, filename, allocator);

        ssize total_size = asset_size_excluding_header + SIZEOF(BinaryAsset);
        b32 success = platform_write_to_file(path, bin_asset, total_size, allocator);

        if (!success) {
            result = false;
            ASSERT(0);
            break;
        } else {
            result = true;
        }
    }

    return result;
}

typedef struct {
    b32 ok;
    int next_args_index;
    String option;
    String option_arg;
} LongOption;

static LongOption try_parse_long_option(char **argv, int argc, int args_index)
{
    LongOption result = {0};
    result.next_args_index = args_index;

    String arg = str_from_c_str(argv[args_index]);

    if (str_starts_with(arg, str("--"))) {
        Cut cut = str_cut(arg, str("="));

        result.option = cut.head;

        if (cut.ok) {
            result.option_arg = cut.tail;
        } else if (args_index < argc) {
            ++result.next_args_index;
            result.option_arg = str_from_c_str(argv[result.next_args_index]);
        }

        if (result.option_arg.data) {
            ASSERT(result.option.data);
            result.ok = true;
        }
    }

    return result;
}

static Args parse_args(int argc, char **argv)
{
    Args result = {0};

    for (int i = 1; i < argc; ++i) {
        LongOption parse_result = try_parse_long_option(argv, argc, i);

        if (parse_result.ok) {
            i = parse_result.next_args_index;

            if (str_equal(parse_result.option, str("--input-dir"))) {
                result.input_dir = parse_result.option_arg;
            } else if (str_equal(parse_result.option, str("--enums-path"))) {
                result.enums_path = parse_result.option_arg;
            } else if (str_equal(parse_result.option, str("--filenames-path"))) {
                result.filenames_path = parse_result.option_arg;
            } else if (str_equal(parse_result.option, str("--output-dir"))) {
                result.output_dir = parse_result.option_arg;
            } else {
                fprintf(stderr, "Unknown option '" FMT_STR "'.\n,",
                    FMT_STR_ARG(parse_result.option));
            }
        }
    }

    if (!result.input_dir.data) {
        fprintf(stderr, "Missing required option '--input-dir'.\n");
    }

    if (!result.enums_path.data) {
        fprintf(stderr, "Missing required option '--enums-path'.\n");
    }

    if (!result.filenames_path.data) {
        fprintf(stderr, "Missing required option '--filenames-path'.\n");
    }

    if (!result.output_dir.data) {
        fprintf(stderr, "Missing required option '--output-dir'.\n");
    }

    result.ok = result.input_dir.data && result.enums_path.data && result.filenames_path.data
                && result.output_dir.data;

    return result;
}

int main(int argc, char **argv)
{
    LinearArena arena = la_create(default_allocator, MB(8));
    Allocator allocator = la_allocator(&arena);

    int result = 0;

    Args args = parse_args(argc, argv);

    if (!args.ok) {
        result = 1;
    } else {
        String assets_definition_path =
            str_concat(args.input_dir, str("/assets.txt"), allocator);

        String source =
            platform_read_entire_file_as_string(assets_definition_path, allocator, &arena);

        if (!source.data) {
            fprintf(stderr, "Could not open file\n");
        } else {
            Record *root = parse(source, &arena);
            ValueList *textures = value_as_list(record_get(root, str("textures")));

            CompiledAssetList compiled_textures =
                compile_textures(textures, args.input_dir, &arena);
            // TODO: error check

            String enums = generate_asset_enums(compiled_textures, &arena, allocator);

            String binary_asset_paths =
                generate_binary_asset_file_list(compiled_textures, &arena, allocator);

            b32 write_result =
                platform_write_to_file(args.enums_path, enums.data, enums.length, allocator)
                && platform_write_to_file(args.filenames_path, binary_asset_paths.data,
                    binary_asset_paths.length, allocator);
            ASSERT(write_result);

            b32 dir_created = platform_create_directory(args.output_dir, allocator);
            ASSERT(dir_created);

            b32 asset_files_written =
                write_binary_asset_files(compiled_textures, args, allocator);
            ASSERT(asset_files_written);
        }
    }

    la_destroy(&arena);

    return result;
}
