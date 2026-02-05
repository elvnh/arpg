#ifndef GAME_H
#define GAME_H

#include "item_system.h"
#include "ui/ui_core.h"
#include "game/magic.h"
#include "platform/platform.h"
#include "world/world.h"
#include "debug.h"
#include "animation.h"
#include "game_ui.h"
#include "asset_table.h"

/*
  TODO:
  - Move hovered_entity into game_state
  - Fix camera panning at start of game
 */

#define GAME_MEMORY_SIZE MB(128)
#define PERMANENT_ARENA_SIZE GAME_MEMORY_SIZE / 2
#define FRAME_ARENA_SIZE GAME_MEMORY_SIZE / 2
#define FREE_LIST_ARENA_SIZE PERMANENT_ARENA_SIZE / 4

struct RenderBatchList;

typedef struct Game {
    // TODO: should these exist in world instead?
    EntitySystem entity_system;
    ItemSystem item_system;

    World world; // currently loaded world
    AssetTable asset_table;
    DebugState debug_state;
    RNGState rng_state;
    GameUIState game_ui;
} Game;

typedef struct GameMemory {
    // For allocations will live for the entire program
    LinearArena permanent_memory;

    // For allocations that are cleared each frame
    LinearArena temporary_memory;

    // For allocations that will need to be allocated and freed randomly, is a subarena of permanent_memory
    FreeListArena free_list_memory;
} GameMemory;

void game_update_and_render(Game *game_state, PlatformCode platform_code, struct RenderBatchList *rbs,
    FrameData frame_data, GameMemory *game_memory);
void game_initialize(Game *game_state, GameMemory *game_memory);

#endif //GAME_H
