#include "loot.h"

#include "base/linear_arena.h"
#include "base/random.h"

typedef struct {
    LootItem items[1024];
    //ssize item_count;
} LootTable;

// clang-format off
// TODO: can this initialization be done in a cleaner way?
static LootTable loot_tables[] = {
    [LOOT_TABLE_GENERIC] = {
        {
            {ITEM_SWORD, 100},
            {ITEM_SHIELD, 50},
        },
    },
};
// clang-format on

static LootTable *get_table_by_id(LootTableID table_id)
{
    ASSERT(table_id >= 0);
    ASSERT(table_id < LOOT_TABLE_COUNT);

    LootTable *result = &loot_tables[table_id];
    return result;
}

static s64 table_total_weight_count(LootTable *table)
{
    s64 result = 0;

    for (ssize i = 0; i < ARRAY_COUNT(table->items); ++i) {
        LootItem item = table->items[i];

        if (item.item == ITEM_NULL) {
            break;
        }

        result += item.weight;
    }

    ASSERT(result > 0);
    return result;
}

Item roll_loot_from_table(LootTableID table_id)
{
    LootTable *table = get_table_by_id(table_id);

    s64 roll = rng_s64(0, table_total_weight_count(table));

    Item result = 0;

    s64 x = 0;
    for (ssize i = 0; i < ARRAY_COUNT(table->items); ++i) {
        LootItem item = table->items[i];

        // Used as null terminator
        if (item.item == ITEM_NULL) {
            break;
        }

        s64 next = x + item.weight;
        if ((roll >= x) && (roll < next)) {
            result = item.item;
        }

        x += next;
    }

    ASSERT(result != ITEM_NULL);

    return result;
}
