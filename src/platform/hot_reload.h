#ifndef HOT_RELOAD_H
#define HOT_RELOAD_H

#include "game/game.h"
#include "platform.h"
#include "renderer/frontend/render_batch.h"

#if HOT_RELOAD
#    define hot_reload_initialize(memory) hot_reload_initialize_impl((memory))
#    define load_game_code(game_code, scratch) load_game_code_impl((game_code), (scratch))
#    define unload_game_code(game_code) unload_game_code_impl((game_code))
#    define reload_game_code_if_recompiled(code, arena) reload_game_code_if_recompiled_impl((code), (arena))
#else
#    define hot_reload_initialize(memory)
#    define load_game_code(game_code, scratch)
#    define unload_game_code(game_code)
#    define reload_game_code_if_recompiled(code, arena)
#endif

typedef void (GameInitialize)(Game *, GameMemory *);
typedef void (GameUpdateAndRender)(Game *, PlatformCode, RenderBatchList *, FrameData, GameMemory *);

typedef struct {
    void *handle;
    GameInitialize *initialize;
    GameUpdateAndRender *update_and_render;
    Timestamp last_load_time;
} GameCode;

GameCode hot_reload_initialize_impl(GameMemory* game_memory);
void     load_game_code_impl(GameCode *game_code, LinearArena *scratch);
void     unload_game_code_impl(GameCode *game_code);
void     reload_game_code_if_recompiled_impl(GameCode *game_code, LinearArena *frame_arena);

#endif //HOT_RELOAD_H
