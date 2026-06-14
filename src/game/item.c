#include "item.h"

#include "asset_table.h"
#include "world/world.h"

// TODO: make this system similar to spells
BaseItemInfo get_base_item_info(BaseItemID id)
{
#define BASE_ITEM(name, spelling, inv_w, inv_h) {spelling, {inv_w, inv_h}, {0}, {0, 0}}
    static BaseItemInfo item_infos[] = {BASE_ITEM_LIST};
#undef BASE_ITEM

    ASSERT(VALID_INDEX_FOR(id, item_infos));

    BaseItemInfo result = item_infos[id];

    return result;
}

Entity *make_entity_from_item(World *world, Item item)
{
    EntityWithID entity_with_id =
        world_spawn_non_spatial_entity(world, FACTION_NEUTRAL, ENTITY_KIND_PERSISTENT);
    Entity *e = entity_with_id.entity;

    BaseItemInfo item_info = get_base_item_info(item.base_item);

    NameComponent *name = es_add_component(e, NameComponent);
    *name = name_component(str_from_c_str(item_info.name));

    InventoryStorable *inv_item = es_add_component(e, InventoryStorable);
    inv_item->inventory_grid_size = item_info.inventory_size;

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (item.base_item) {
        case ITEM_SWORD: {
            SpriteComponent *sprite = es_add_component(e, SpriteComponent);
            sprite->sprite.texture = texture_handle(ICE_SHARD_TEXTURE);
            sprite->sprite.color = RGBA32_WHITE;
            sprite->sprite.size = v2(32, 32);
        } break;

            INVALID_CASE(ITEM_COUNT);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    ItemModifiers *mods = es_add_component(e, ItemModifiers);
    *mods = item.modifiers;

    return e;
}
