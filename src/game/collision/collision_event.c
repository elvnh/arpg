#include "collision_event.h"
#include "base/free_list_arena.h"
#include "base/sl_list.h"
#include "world/world.h"

#define INTERSECTION_TABLE_ARENA_SIZE (INTERSECTION_TABLE_SIZE * SIZEOF(CollisionEvent) * 2)
#define INTERSECTION_TABLE_SIZE 512

CollisionEventTable collision_event_table_create(FreeListArena *parent_arena)
{
    CollisionEventTable result = {0};
    result.table = fl_alloc_array(parent_arena, CollisionEventList, INTERSECTION_TABLE_SIZE);
    result.table_size = INTERSECTION_TABLE_SIZE;
    result.arena = la_create(fl_allocator(parent_arena), INTERSECTION_TABLE_ARENA_SIZE);

    return result;
}

CollisionEvent *collision_event_table_find(CollisionEventTable *table, EntityID a, EntityID b)
{
    EntityPair searched_pair = unordered_entity_pair(a, b);
    u64 hash = entity_pair_hash(searched_pair);
    ssize index = mod_index(hash, table->table_size);

    CollisionEventList *list = &table->table[index];

    CollisionEvent *node = 0;
    for (node = list_head(list); node; node = list_next(node)) {
        if (entity_pair_equal(searched_pair, node->ordered_entity_pair)) {
            break;
        }
    }

    return node;
}

void collision_event_table_insert(CollisionEventTable *table, EntityID a, EntityID b)
{
    if (!collision_event_table_find(table, a, b)) {
        EntityPair entity_pair = unordered_entity_pair(a, b);
        u64 hash = entity_pair_hash(entity_pair);
        ssize index = mod_index(hash, table->table_size);

        CollisionEvent *new_node = la_allocate_item(&table->arena, CollisionEvent);
        new_node->ordered_entity_pair = entity_pair;

        CollisionEventList *list = &table->table[index];
        sl_list_push_back(list, new_node);
    } else {
        ASSERT(0);
    }
}

b32 entities_intersected_this_frame(struct World *world, EntityID a, EntityID b)
{
    b32 result = collision_event_table_find(&world->current_frame_collisions, a, b) != 0;

    return result;
}

b32 entities_intersected_previous_frame(struct World *world, EntityID a, EntityID b)
{
    b32 result = collision_event_table_find(&world->previous_frame_collisions, a, b) != 0;

    return result;
}
