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

typedef enum {
    GRID_COORD_TRUNCATE,
    GRID_COORD_ROUND_TO_NEAREST,
} GridCoordRounding;

static inline Vector2i screen_to_inventory_grid_coords(
    Vector2 position, Rectangle inventory_grid_rect, GridCoordRounding rounding)
{
    Vector2 screen_offset = v2_sub(position, inventory_grid_rect.position);
    Vector2 floating_grid_pos = v2_div_s(screen_offset, INVENTORY_GRID_UI_CELL_SIZE);

    if (rounding == GRID_COORD_ROUND_TO_NEAREST) {
        floating_grid_pos.x = roundf(floating_grid_pos.x);
        floating_grid_pos.y = roundf(floating_grid_pos.y);
    }

    Vector2i result = v2_to_v2i(floating_grid_pos);

    return result;
}

static inline Vector2 inventory_grid_to_screen_vector(Vector2i grid_pos)
{
    Vector2 result = {(f32)grid_pos.x * INVENTORY_GRID_UI_CELL_SIZE,
        (f32)grid_pos.y * INVENTORY_GRID_UI_CELL_SIZE};

    return result;
}

static inline Vector2 inventory_grid_to_screen_coords(
    Vector2i position, Rectangle inventory_grid_rect)
{
    Vector2 result = inventory_grid_to_screen_vector(position);
    result = v2_add(result, inventory_grid_rect.position);

    return result;
}

#endif // INVENTORY_MENU_H
