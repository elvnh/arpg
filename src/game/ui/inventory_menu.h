#ifndef INVENTORY_MENU_H
#define INVENTORY_MENU_H

#include "base/rectangle.h"

// TODO: make these resize with screen instead
#define INVENTORY_GRID_UI_CELL_SIZE 32
#define INVENTORY_GRID_UI_SIZE (Vector2) {                                       \
            (f32)INVENTORY_GRID_CELL_COUNTS.x * (f32)INVENTORY_GRID_UI_CELL_SIZE,     \
            (f32)INVENTORY_GRID_CELL_COUNTS.y * (f32)INVENTORY_GRID_UI_CELL_SIZE      \
       }

struct Game;
struct FrameData;
struct LinearArena;
struct RenderBatch;

typedef struct {
    b32 active;

    Rectangle inventory_grid_rect;
    b32 can_interact_with_inventory;
} InventoryMenu;

void inventory_menu(
    struct Game *game, const struct FrameData *frame_data, struct LinearArena *scratch);

#endif // INVENTORY_MENU_H
