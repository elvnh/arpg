#include "loot.h"

#include "base/linear_arena.h"
#include "base/random.h"
#include "components/modifier.h"

typedef struct {
    ItemID item;
    ssize weight;
} LootTableRow;

typedef struct {
    s64 weight;
    Stat target_stat;
    NumericModifierType modifier_type;
    StatValue min_roll;
    StatValue max_roll;
} ModifierTableRow;

typedef struct {
    LootTableRow *data;
    ssize count;
    ssize total_weight;
} LootTable;

typedef struct {
    ModifierTableRow *data;
    ssize count;
    ssize total_weight;
} ModifierTable;

// TODO: move these into functions
// clang-format off
static LootTableRow loot_tables[][LOOT_TABLE_COUNT] = {
    [LOOT_TABLE_GENERIC] = {
        {ITEM_SWORD, 100},
    },
};

static ModifierTableRow modifier_tables[][ITEM_COUNT] = {
    [ITEM_SWORD] = {
        {100, STAT_FIRE_DAMAGE, NUMERIC_MOD_FLAT_ADDITIVE, 100, 200},
    },
};
// clang-format on

static LootTable get_loot_table(LootTableID id, LinearArena *arena)
{
    /* ASSERT(arena); */
    ASSERT(id >= 0);
    ASSERT(id < LOOT_TABLE_COUNT);

    LootTable result = {0};
    result.data = loot_tables[id];
    result.count = ARRAY_COUNT(loot_tables[id]);

    for (ssize i = 0; i < result.count; ++i) {
        result.total_weight += result.data[i].weight;
    }

    return result;
}

static ModifierTable get_modifier_table_for_item(ItemID item_id, LinearArena *arena)
{
    ASSERT(item_id >= 0);
    ASSERT(item_id < ITEM_COUNT);

    ModifierTable result = {0};
    result.data = modifier_tables[item_id];
    result.count = ARRAY_COUNT(modifier_tables[item_id]);

    for (ssize i = 0; i < result.count; ++i) {
        result.total_weight += result.data[i].weight;
    }

    return result;
}

static ItemModifiers roll_modifiers_for_item(ItemID item_id)
{
    ASSERT(item_id >= 0);
    ASSERT(item_id < ITEM_COUNT);

    ItemModifiers result = {0};

    ModifierTable table = get_modifier_table_for_item(item_id, 0);

    s32 mods_to_roll = rng_s32(1, 6);

    for (s32 i = 0; i < mods_to_roll; ++i) {
        s64 x = 0;
        s64 roll = rng_s64(0, table.total_weight);
        ModifierTableRow rolled_row = {0};

        for (ssize j = 0; j < table.count; ++j) {
            ModifierTableRow row = table.data[j];
            s64 next = x + row.weight;

            // TODO: make sure we can't roll same mods multiple times
            if ((roll >= x) && (roll < next)) {
                rolled_row = row;
                break;
            }

            x = next;
        }

        ASSERT(rolled_row.min_roll > 0);
        ASSERT(rolled_row.max_roll > 0);

        s64 stat_value = rng_s64(rolled_row.min_roll, rolled_row.max_roll);
        Modifier mod = {rolled_row.target_stat, stat_value, rolled_row.modifier_type};
        result.modifiers[result.modifier_count++] = mod;
    }

    return result;
}

Item roll_loot_from_table(LootTableID table_id)
{
    LootTable table = get_loot_table(table_id, 0);

    s64 roll = rng_s64(0, table.total_weight);

    ItemID base_item = 0;

    s64 x = 0;
    for (ssize i = 0; i < table.count; ++i) {
        LootTableRow item = table.data[i];

        s64 next = x + item.weight;
        if ((roll >= x) && (roll < next)) {
            base_item = item.item;
            break;
        }

        x = next;
    }

    ItemModifiers mods = roll_modifiers_for_item(base_item);
    Item result = {base_item, mods};

    return result;
}
