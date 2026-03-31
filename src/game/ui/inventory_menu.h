#ifndef INVENTORY_MENU_H
#define INVENTORY_MENU_H

#include "base/rectangle.h"
#include "base/string8.h"
#include "command.h"
#include "entity/entity_id.h"
#include "platform/input_event.h"

// TODO: make these resize with screen instead
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

// TODO: generalize this so that it can handle shop interfaces and such too
typedef struct {
    EntityID item_on_cursor;

    b32 active;
    Rectangle inventory_grid_rect;
    b32 can_interact_with_inventory;
} InventoryMenu;

void inventory_menu_update(InventoryMenu *inv_menu);
void inventory_menu(struct UIState *ui, InventoryMenu *inv_menu, struct World *world,
    InputEvents *input, CommandQueue *commands, LinearArena *scratch);

// TODO: once the equipment and inventory menu are unified, these don't need to be in header
String get_item_name_widget_text(struct Entity *item_entity, struct LinearArena *arena);
void item_hover_menu(struct UIState *ui, struct Entity *item, Vector2 mouse_position,
    struct LinearArena *arena);

#endif // INVENTORY_MENU_H
