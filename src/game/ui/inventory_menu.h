#ifndef INVENTORY_MENU_H
#define INVENTORY_MENU_H

#include "base/rectangle.h"
#include "entity/entity_id.h"

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
struct World;
struct UIState;

// TODO: generalize this so that it can handle shop interfaces and such too
typedef struct {
    EntityID item_on_cursor;

    b32 active;
    Rectangle inventory_grid_rect;
    b32 can_interact_with_inventory;
} InventoryMenu;

void inventory_menu(struct UIState *ui, InventoryMenu *inv_menu, struct World *world,
    const struct FrameData *frame_data, struct LinearArena *scratch);

// TODO: should this be called from within inventory_menu?
void render_item_on_cursor(InventoryMenu *inv_menu, struct World *world,
    struct RenderBatch *rb, Vector2 mouse_pos, struct LinearArena *arena);
void pick_up_item_from_world_and_put_on_cursor(
    InventoryMenu *inv_menu, struct World *world, struct Entity *item_entity);

#endif // INVENTORY_MENU_H
