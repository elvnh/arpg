#include "item.h"

#include "world/world.h"

// TODO: make this system similar to spells

Entity *make_entity_from_item(World *world, Item item)
{
    // NOTE: we don't set a particular position, this function just handles
    // creating item, not dropping it into world
    // TODO: spawn as non-spatial?
    EntityWithID entity_with_id =
        world_spawn_entity(world, V2_ZERO, FACTION_NEUTRAL, ENTITY_KIND_PERSISTENT);
    Entity *e = entity_with_id.entity;

    BEGIN_EXHAUSTIVE_SWITCH;
    switch (item) {
        case ITEM_SWORD: {
            // TODO: add all components
            ASSERT(0);
        } break;

        case ITEM_SHIELD: {
            // TODO: add all components
            ASSERT(0);
        } break;

            INVALID_CASE(ITEM_NULL);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    return e;
}
