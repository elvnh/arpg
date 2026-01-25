#include "health.h"
#include "stats.h"
#include "world.h"

Health create_health_instance(World *world, Entity *entity)
{
    StatValue max_hp = get_total_stat_value(entity, STAT_HEALTH, world->item_system);
    ASSERT(max_hp > 0);

    Health result = {0};
    result.max_hitpoints = max_hp;
    result.current_hitpoints = max_hp;

    return result;
}
