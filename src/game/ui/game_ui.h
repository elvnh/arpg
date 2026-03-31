#ifndef GAME_UI_H
#define GAME_UI_H

#include "command.h"
#include "entity/entity.h"
#include "inventory_menu.h"
#include "platform/input_event.h"
#include "ui/ui_builder.h"

struct Game;
struct LinearArena;
struct World;
struct RenderBatch;

typedef struct GameUI {
    UIState backend_state;

    EntityID hovered_entity;

    InventoryMenu inventory_menu;

    ssize selected_spellbook_index;

} GameUI;

void game_ui(struct Game *game, struct LinearArena *scratch, InputEvents *input,
    CommandQueue *commands);

#endif //GAME_UI_H
