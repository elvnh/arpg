#include "entity.h"
#include "base/utils.h"

static b32 entity_id_is_valid(EntityStorage *es, EntityID id)
{
    b32 result = (id.id < es->entity_count);
    return result;
}

EntityID es_create_entity(EntityStorage *es)
{
    EntityID_Type id = es->entity_count++;
    es->entities[id] = (Entity){0};

    return (EntityID){id};
}

Entity  *es_get_entity(EntityStorage *es, EntityID id)
{
    ASSERT(entity_id_is_valid(es, id));
    Entity *result = &es->entities[id.id];

    return result;
}
