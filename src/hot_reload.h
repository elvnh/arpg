#ifndef HOT_RELOAD_H
#define HOT_RELOAD_H

#include "game/game.h"
#include "platform.h"

typedef void (GameInitialize)(GameState *, GameMemory *);
typedef void (GameUpdateAndRender)(GameState *, PlatformCode, RenderBatchList *, FrameData, GameMemory *);

typedef struct {
    void *handle;
    GameInitialize *initialize;
    GameUpdateAndRender *update_and_render;
    String game_library_path;
    String lock_file_path;
    Timestamp last_load_time;
} GameCode;

GameCode hot_reload_initialize(GameMemory* game_memory);
void     load_game_code(GameCode *game_code, LinearArena *scratch);
void     unload_game_code(GameCode *game_code);
void     reload_game_code_if_recompiled(GameCode *game_code, LinearArena *frame_arena);

#endif //HOT_RELOAD_H
