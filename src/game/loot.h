#ifndef LOOT_H
#define LOOT_H

#include "base/typedefs.h"
#include "item.h"

#define LOOT_TIME_BETWEEN_DROPS 0.15f

struct LinearArena;

typedef enum {
    LOOT_TABLE_GENERIC,
    LOOT_TABLE_COUNT,
} LootTableID;

Item roll_loot_from_table(LootTableID table_id, struct LinearArena *scratch);

#endif // LOOT_H
