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

static void generate_asset_enum(CompiledAssetList assets, String enum_name, String macro_name,
    StringBuilder *sb, LinearArena *arena)
{
    String enum_prefix = str_to_upper(macro_name, la_allocator(arena));

    str_builder_append(sb, str("#pragma once\n\n"));

    str_builder_append(sb, format(arena, "#define " FMT_STR "(a) " FMT_STR "_##a",
                               FMT_STR_ARG(macro_name), FMT_STR_ARG(enum_prefix)));
    str_builder_append(sb, str("\n\n"));
    str_builder_append(sb, str("typedef enum {"));

    for (CompiledAsset *a = list_head(&assets); a; a = list_next(a)) {
        String s = format(arena, "\n    " FMT_STR "(" FMT_STR "),", FMT_STR_ARG(macro_name),
            FMT_STR_ARG(a->name));

        str_builder_append(sb, s);
    }

    str_builder_append(sb, format(arena, "\n} " FMT_STR ";", FMT_STR_ARG(enum_name)));
}

static String generate_asset_enums(CompiledAssetList textures, LinearArena *arena,
    Allocator allocator)
{
    StringBuilder sb = str_builder_allocate(MB(8), allocator);

    generate_asset_enum(textures, str("TextureHandle"), str("texture_handle"), &sb, arena);

    return sb.buffer;
}

static String generate_binary_asset_paths(CompiledAssetList textures, LinearArena *arena,
    Allocator allocator)
{
    StringBuilder sb = str_builder_allocate(MB(8), allocator);

    str_builder_append(&sb, str("static const char *binary_asset_paths[] = {"));

    for (CompiledAsset *a = list_head(&textures); a; a = list_next(a)) {
        String output_path = str_concat(a->name, str(".dat"), allocator);

        String element = format(arena, "\n    \"" FMT_STR "\",", FMT_STR_ARG(output_path));
        str_builder_append(&sb, element);
    }

    str_builder_append(&sb, str("\n}"));

    return sb.buffer;
}

#define GENERATED_ENUMS_PATH str("generated/asset_handle.h")
#define GENERATED_FILES_PATH str("generated/binary_asset_paths.c")

int main(int argc, char **argv)
{
    ASSERT(argc > 2);

    LinearArena arena = la_create(default_allocator, MB(8));
    Allocator allocator = la_allocator(&arena);

    String assets_definition_path = str_from_c_str(argv[1]);
    String assets_dir = str_from_c_str(argv[2]);

    String source =
        platform_read_entire_file_as_string(assets_definition_path, allocator, &arena);

    if (!source.data) {
        fprintf(stderr, "Could not open file\n");
    } else {
        Record *root = parse(source, &arena);
        ValueList *textures = value_as_list(record_get(root, str("textures")));

        CompiledAssetList compiled_textures = compile_textures(textures, assets_dir, &arena);
        // TODO: error check

        String enums = generate_asset_enums(compiled_textures, &arena, allocator);

        b32 write_result =
            platform_write_to_file(GENERATED_ENUMS_PATH, enums.data, enums.length, allocator);

        String binary_asset_paths =
            generate_binary_asset_paths(compiled_textures, &arena, allocator);

        write_result &= platform_write_to_file(GENERATED_FILES_PATH, binary_asset_paths.data,
            binary_asset_paths.length, allocator);

        ASSERT(write_result);
        // generate enums
        // serialize to files
        // generate list of binary files for asset loader to load
    }

    la_destroy(&arena);

    return 0;
}
