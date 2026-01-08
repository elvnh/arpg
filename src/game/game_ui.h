#ifndef GAME_UI_H
#define GAME_UI_H

#include "ui/ui_builder.h"
#include "entity.h"

struct Game;
struct GameMemory;
struct FrameData;

typedef struct GameUIState {
    UIState backend_state;

    EntityID hovered_entity;
    b32 inventory_menu_open;
    ssize selected_spellbook_index;
} GameUIState;

void game_ui(struct Game *game, struct GameMemory *game_memory, const struct FrameData *frame_data);

#endif //GAME_UI_H
