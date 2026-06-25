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

static void define_asset_list_enum(CompiledAssetList assets, String enum_name,
    String macro_name, StringBuilder *sb, LinearArena *arena)
{
    String enum_prefix = str_to_upper(macro_name, la_allocator(arena));

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

        StringBuilder sb = str_builder_allocate(MB(8), allocator);

        define_asset_list_enum(compiled_textures, str("TextureHandle"), str("texture_handle"),
            &sb, &arena);

        str_print(sb.buffer);

        // generate enums
        // serialize to files
        // generate list of binary files for asset loader to load
    }

    la_destroy(&arena);

    return 0;
}
