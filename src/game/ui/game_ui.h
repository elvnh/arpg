#ifndef GAME_UI_H
#define GAME_UI_H

#include "entity/entity.h"
#include "inventory_menu.h"
#include "ui/ui_builder.h"

struct Game;
struct LinearArena;
struct FrameData;
struct World;
struct RenderBatch;

typedef struct GameUIState {
    UIState backend_state;

    EntityID hovered_entity;

    InventoryMenu inventory_menu;

    ssize selected_spellbook_index;

} GameUIState;

void game_ui(
    struct Game *game, struct LinearArena *scratch, const struct FrameData *frame_data);

#endif //GAME_UI_H
