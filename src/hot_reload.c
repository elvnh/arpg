#include <dlfcn.h>

#include "hot_reload.h"

#define BEGIN_IGNORE_FUNCTION_PTR_WARNINGS              \
    _Pragma("GCC diagnostic push");                     \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"");
#define END_IGNORE_FUNCTION_PTR_WARNINGS _Pragma("GCC diagnostic pop")

void load_game_code(GameCode *game_code, LinearArena *scratch)
{
    String bin_dir = platform_get_parent_path(game_code->game_library_path, la_allocator(scratch), scratch);

    {
        String lock_file_path = str_concat(bin_dir, str_lit("/build/lock"), la_allocator(scratch));
        while (platform_file_exists(lock_file_path, scratch));
    }

    void *handle = dlopen(game_code->game_library_path.data, RTLD_NOW);

    if (!handle) {
        goto error;
    }

    void *initialize = dlsym(handle, "game_initialize");
    void *update_and_render = dlsym(handle, "game_update_and_render");

    if (!initialize || !update_and_render) {
        goto error;
    }

    ASSERT(initialize);
    ASSERT(update_and_render);

    BEGIN_IGNORE_FUNCTION_PTR_WARNINGS;

    game_code->handle = handle;
    game_code->initialize = initialize;
    game_code->update_and_render = (GameUpdateAndRender *)update_and_render;

    END_IGNORE_FUNCTION_PTR_WARNINGS;

    game_code->last_load_time = platform_get_time();

    return;

  error:
    fprintf(stderr, "%s\n", dlerror());
    ASSERT(0);
}

void unload_game_code(GameCode *game_code)
{
    game_code->update_and_render = 0;
    dlclose(game_code->handle);
    game_code->handle = 0;
}

void reload_game_code_if_recompiled(GameCode *game_code, LinearArena *frame_arena)
{
    Timestamp so_mod_time = platform_get_file_info(game_code->game_library_path, frame_arena).last_modification_time;

    if (timestamp_less_than(game_code->last_load_time, so_mod_time)) {
        unload_game_code(game_code);
        load_game_code(game_code, frame_arena);

        printf("Hot reloaded game.\n");
    }
}
