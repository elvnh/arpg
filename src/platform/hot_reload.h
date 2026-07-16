#ifndef HOT_RELOAD_H
#define HOT_RELOAD_H

#include "game/game.h"
#include "platform.h"
#include "renderer/frontend/render_batch.h"

#if HOT_RELOAD
#    define hot_reload_initialize()       hot_reload_initialize_impl()
#    define load_game_code(game_code)     load_game_code_impl((game_code))
#    define unload_game_code(game_code)   unload_game_code_impl((game_code))
#    define reload_game_code_if_recompiled(code)                                  \
        reload_game_code_if_recompiled_impl((code))
#else
#    define hot_reload_initialize(memory)
#    define load_game_code(game_code)
#    define unload_game_code(game_code)
#    define reload_game_code_if_recompiled(code, arena)
#endif

typedef void(GameInitialize)(Game *, GameMemory *);
typedef void(
    GameUpdateAndRender)(Game *, PlatformCode, RenderBatchList *, FrameInput *, GameMemory *);

typedef struct {
    void *handle;
    GameInitialize *initialize;
    GameUpdateAndRender *update_and_render;
    Timestamp last_load_time;
} GameCode;

GameCode hot_reload_initialize_impl(void);
void load_game_code_impl(GameCode *game_code);
void unload_game_code_impl(GameCode *game_code);
void reload_game_code_if_recompiled_impl(GameCode *game_code);

#endif //HOT_RELOAD_H
