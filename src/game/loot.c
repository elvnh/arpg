#include "loot.h"

#include "base/linear_arena.h"
#include "base/random.h"
#include "components/modifier.h"

typedef struct {
    s64 weight;
    Stat target_stat;
    NumericModifierType modifier_type;
    StatValue min_roll;
    StatValue max_roll;
} ModifierTableRow;

// clang-format off
static LootItem loot_tables[][LOOT_TABLE_COUNT] = {
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

static s64 mod_table_total_weight(ItemID item)
{
    s64 result = 0;

    for (ssize i = 0; i < ARRAY_COUNT(modifier_tables[item]); ++i) {
        result += modifier_tables[item][i].weight;
    }

    return result;
}

static ItemModifiers roll_modifiers_for_item(ItemID item)
{
    ASSERT(item >= 0);
    ASSERT(item < ITEM_COUNT);

    ItemModifiers result = {0};

    ModifierTableRow *table = modifier_tables[item];

    s32 mods_to_roll = rng_s32(1, 6);

    for (s32 i = 0; i < mods_to_roll; ++i) {
        s64 x = 0;
        s64 roll = rng_s64(0, mod_table_total_weight(item));
        ModifierTableRow rolled_row = {0};

        for (ssize j = 0; j < ARRAY_COUNT(modifier_tables[item]); ++j) {
            ModifierTableRow row = table[j];
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

static LootItem *get_table_by_id(LootTableID table_id)
{
    ASSERT(table_id >= 0);
    ASSERT(table_id < LOOT_TABLE_COUNT);

    LootItem *result = loot_tables[table_id];
    return result;
}

static s64 table_total_weight_count(LootTableID table)
{
    s64 result = 0;

    for (ssize i = 0; i < ARRAY_COUNT(loot_tables[table]); ++i) {
        LootItem item = loot_tables[table][i];

        result += item.weight;
    }

    ASSERT(result > 0);
    return result;
}

Item roll_loot_from_table(LootTableID table_id)
{
    LootItem *table = get_table_by_id(table_id);

    s64 roll = rng_s64(0, table_total_weight_count(table_id));

    ItemID base_item = 0;

    s64 x = 0;
    for (ssize i = 0; i < ARRAY_COUNT(loot_tables[table_id]); ++i) {
        LootItem item = table[i];

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
