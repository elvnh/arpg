#ifndef GAME_UI_H
#define GAME_UI_H

#include "ui/ui_builder.h"

typedef struct {
    UIState backend_state;

    b32 inventory_menu_open;
} GameUIState;

struct GameState;
struct GameMemory;
struct FrameData;

void game_ui(struct GameState *game_state, struct GameMemory *game_memory, const struct FrameData *frame_data);

#endif //GAME_UI_H
