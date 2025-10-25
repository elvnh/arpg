#include <dlfcn.h>

#include "base/string8.h"
#include "hot_reload.h"

#define GAME_SO_NAME "libgame.so"

#define BEGIN_IGNORE_FUNCTION_PTR_WARNINGS              \
    _Pragma("GCC diagnostic push");                     \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"");
#define END_IGNORE_FUNCTION_PTR_WARNINGS _Pragma("GCC diagnostic pop")

GameCode hot_reload_initialize(GameMemory* game_memory)
{
    GameCode result = {0};

    Allocator allocator = la_allocator(&game_memory->permanent_memory);

    String executable_dir = platform_get_executable_directory(allocator, &game_memory->temporary_memory);
    String so_path = str_concat(executable_dir, str_lit("/"GAME_SO_NAME), allocator);
    String lock_file_path = str_concat(executable_dir, str_lit("/build/lock"),
        la_allocator(&game_memory->permanent_memory));

    result.game_library_path = str_null_terminate(so_path, allocator);
    result.lock_file_path  = str_null_terminate(lock_file_path, allocator);

    load_game_code(&result, &game_memory->temporary_memory);

    return result;
}

void load_game_code(GameCode *game_code, LinearArena *scratch)
{
    // NOTE: this loop should only be entered if game was recompiled since checking
    // this before the call
    while (platform_file_exists(game_code->lock_file_path, scratch));
    ASSERT(dlopen(game_code->game_library_path.data, RTLD_NOLOAD) == 0);
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
    game_code->update_and_render = update_and_render;

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

    if (timestamp_less_than(game_code->last_load_time, so_mod_time)
        && !platform_file_exists(game_code->lock_file_path, frame_arena)) {
        unload_game_code(game_code);
        load_game_code(game_code, frame_arena);

        printf("Hot reloaded game.\n");
    }
}
