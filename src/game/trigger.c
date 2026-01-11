#include "trigger.h"
#include "base/utils.h"
#include "base/sl_list.h"
#include "base/free_list_arena.h"
#include "entity_id.h"
#include "world.h"

static inline ssize trigger_cooldown_hashed_index(TriggerCooldownTable *table, EntityID self, EntityID other)
{
    // TODO: better hash
    u64 hash = entity_id_hash(self) ^ entity_id_hash(other);
    ssize result = mod_index(hash, ARRAY_COUNT(table->table));

    return result;
}

static TriggerCooldown *find_trigger_cooldown(TriggerCooldownTable *table, EntityID self, EntityID other,
    ComponentBitset component_id)
{
    ASSERT(!entity_id_equal(self, other));

    ssize index = trigger_cooldown_hashed_index(table, self, other);

    TriggerCooldownList *list = &table->table[index];

    TriggerCooldown *current;
    for (current = list_head(list); current; current = list_next(current)) {
	b32 is_match = entity_id_equal(current->owning_entity, self)
            && entity_id_equal(current->collided_entity, other)
	    && (current->owning_component == component_id);

        if (is_match) {
            break;
        }
    }

    return current;
}

// TODO: allow setting multiple components
void add_trigger_cooldown(TriggerCooldownTable *table, EntityID self, EntityID other,
    ComponentBitset component, RetriggerBehaviour retrigger_behaviour, FreeListArena *arena)
{
    ASSERT(!entity_id_equal(self, other));

    if (!find_trigger_cooldown(table, self, other, component)) {
        TriggerCooldown *cooldown = list_head(&table->free_node_list);

        if (!cooldown) {
            cooldown = fl_alloc_item(arena, TriggerCooldown);
        } else {
            list_pop_head(&table->free_node_list);
        }

        *cooldown = zero_struct(TriggerCooldown);
        cooldown->owning_entity = self;
        cooldown->collided_entity = other;
	cooldown->retrigger_behaviour = retrigger_behaviour;
	cooldown->owning_component = component;

        ssize index = trigger_cooldown_hashed_index(table, self, other);
        list_push_back(&table->table[index], cooldown);
    }
}
void remove_expired_trigger_cooldowns(World *world, EntitySystem *es)
{
    for (ssize i = 0; i < ARRAY_COUNT(world->trigger_cooldowns.table); ++i) {
        TriggerCooldownList *cd_list = &world->trigger_cooldowns.table[i];

        for (TriggerCooldown *curr_cd = list_head(cd_list); curr_cd;) {
            TriggerCooldown *next_cd = curr_cd->next;

            Entity *owning = es_get_entity(es, curr_cd->owning_entity);
            Entity *other = es_get_entity(es, curr_cd->collided_entity);
            ASSERT(owning);
            ASSERT(other);

            EntityID owning_id = curr_cd->owning_entity;
            EntityID other_id = curr_cd->collided_entity;

	    RetriggerBehaviour retrigger_behaviour = curr_cd->retrigger_behaviour;
            ASSERT(retrigger_behaviour != RETRIGGER_WHENEVER
		  && "A trigger that can retrigger whenever shouldn't be found in cooldown list");

            b32 should_remove = (owning->is_inactive || other->is_inactive)
                || ((retrigger_behaviour == RETRIGGER_AFTER_NON_CONTACT)
                    && !entities_intersected_this_frame(world, owning_id, other_id));

            if (should_remove) {
                list_remove(cd_list, curr_cd);
                list_push_back(&world->trigger_cooldowns.free_node_list, curr_cd);
            }

            curr_cd = next_cd;
        }
    }
}

b32 trigger_is_on_cooldown(TriggerCooldownTable *table, EntityID a, EntityID b, ComponentBitset component)
{
    b32 result = find_trigger_cooldown(table, a, b, component) != 0;

    return result;
}
