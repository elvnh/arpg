#ifndef INVENTORY_MENU_H
#define INVENTORY_MENU_H

#include "base/rectangle.h"
#include "base/string8.h"
#include "command.h"
#include "components/equipment.h"
#include "entity/entity_id.h"
#include "platform/input_event.h"

#define INVENTORY_GRID_UI_CELL_SIZE 32
#define INVENTORY_GRID_UI_SIZE (Vector2) {                                       \
            (f32)INVENTORY_GRID_CELL_COUNTS.x * (f32)INVENTORY_GRID_UI_CELL_SIZE,     \
            (f32)INVENTORY_GRID_CELL_COUNTS.y * (f32)INVENTORY_GRID_UI_CELL_SIZE      \
       }

struct Game;
struct LinearArena;
struct RenderBatch;
struct Entity;
struct World;
struct UIState;

/* TODO:
 * - Generalize this so that it can handle shop interfaces and such too
 * - Allow ctrl-clicking to exchange items even when there is an item on cursor
 */

typedef struct {
    EntityID item_on_cursor;

    b32 active;
    Rectangle inventory_grid_rect;
    b32 can_interact_with_inventory;

    Rectangle equipment_slot_rects[EQUIP_SLOT_COUNT];
} InventoryMenu;

void inventory_menu_update(InventoryMenu *inv_menu);
void inventory_menu(struct UIState *ui, InventoryMenu *inv_menu, struct World *world,
    InputEvents *input, CommandQueue *commands, LinearArena *scratch);

#endif // INVENTORY_MENU_H
