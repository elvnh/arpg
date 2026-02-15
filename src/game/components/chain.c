#include "chain.h"
#include "base/utils.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"

void append_chain_link(EntitySystem *es, ChainComponent *root, ChainComponent *next)
{
    Entity *root_entity = es_get_component_owner(es, root, ChainComponent);
    Entity *link_entity = es_get_component_owner(es, next, ChainComponent);
    ASSERT(root_entity != link_entity);

    root->next_link_entity_id = link_entity->id;

    if (entity_id_is_null(root->chain_root_id)) {
        // This is the root, set it as parent root of appended link
        next->chain_root_id = root_entity->id;
    } else {
        next->chain_root_id = root->chain_root_id;
    }
}

b32 has_chained_off_entity(EntitySystem *es, ChainComponent *root, EntityID id)
{
    ASSERT(entity_id_is_null(root->chain_root_id) && "This has to be the chain root");

    b32 result = false;

    Entity *current_entity = es_get_component_owner(es, root, ChainComponent);

    while (current_entity) {
        ChainComponent *current_link = es_get_component(current_entity, ChainComponent);
        ASSERT(current_link);

        if (entity_id_equal(current_link->chained_off_entity_id, id)) {
            result = true;
            break;
        }

        current_entity = get_next_entity_in_chain(es, current_link);
    }

    return result;
}

Entity *get_next_entity_in_chain(EntitySystem *es, ChainComponent *link)
{
    Entity *result = es_try_get_entity(es, link->next_link_entity_id);
    return result;
}
