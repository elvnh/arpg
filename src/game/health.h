#ifndef HEALTH_H
#define HEALTH_H

#include "stats.h"

struct Entity;
struct World;

typedef struct {
    StatValue max_hitpoints;
    StatValue current_hitpoints;
} Health;

Health create_health_instance(struct World *world, struct Entity *entity);

static inline void set_current_health(Health *health, StatValue new_current_value)
{
    StatValue final_value = CLAMP(new_current_value, 0, health->max_hitpoints);
    health->current_hitpoints = final_value;
}

static inline void set_max_health(Health *health, StatValue new_max_value)
{
    ASSERT(new_max_value > 0);

    health->max_hitpoints = new_max_value;
    health->current_hitpoints = MIN(health->current_hitpoints, health->max_hitpoints);
}

#endif //HEALTH_H
