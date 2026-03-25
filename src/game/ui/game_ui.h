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

    EntityID item_on_cursor;
} GameUIState;

void game_ui(
    struct Game *game, struct LinearArena *scratch, const struct FrameData *frame_data);

// TODO: move to inventory_menu?
void render_item_on_cursor(GameUIState *ui, struct World *world, struct RenderBatch *rb,
    Vector2 mouse_pos, struct LinearArena *arena);
//void put_item_on_cursor(GameUIState *game_ui, struct World *world, Entity *item_entity);

#endif //GAME_UI_H
