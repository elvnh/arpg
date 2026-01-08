#ifndef GAME_UI_H
#define GAME_UI_H

#include "ui/ui_builder.h"

typedef struct GameUIState {
    UIState backend_state;

    b32 inventory_menu_open;
    ssize selected_spellbook_index;
} GameUIState;

struct Game;
struct GameMemory;
struct FrameData;

void game_ui(struct Game *game, struct GameMemory *game_memory, const struct FrameData *frame_data);

#endif //GAME_UI_H
