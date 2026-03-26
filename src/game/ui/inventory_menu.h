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
struct Entity;

typedef struct {
    b32 active;

    Rectangle inventory_grid_rect;
    b32 can_interact_with_inventory;
} InventoryMenu;

void inventory_menu(
    struct Game *game, const struct FrameData *frame_data, struct LinearArena *scratch);
void render_item_on_cursor(
    struct Game *game, struct RenderBatch *rb, Vector2 mouse_pos, struct LinearArena *arena);
void put_item_on_cursor(struct Game *game, struct Entity *item_entity);

#endif // INVENTORY_MENU_H
