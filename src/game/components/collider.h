#ifndef COLLIDER_H
#define COLLIDER_H

typedef enum {
    ON_COLLIDE_STOP,
    ON_COLLIDE_PASS_THROUGH, // TODO: remove, absence of blocking effect is interpreted as passing through
    ON_COLLIDE_BOUNCE,
    ON_COLLIDE_DEAL_DAMAGE,
    ON_COLLIDE_DIE,
    //ON_COLLIDE_EXPLODE,
} CollideEffectKind;

typedef enum {
    OBJECT_KIND_ENTITIES = (1 << 0),
    OBJECT_KIND_TILES = (1 << 1),
} ObjectKind;

typedef enum {
    COLL_RETRIGGER_WHENEVER,
    COLL_RETRIGGER_NEVER,
    COLL_RETRIGGER_AFTER_NON_CONTACT,
} CollisionRetriggerBehaviour;

typedef struct {
    CollideEffectKind kind;
    ObjectKind affects_object_kinds;
    b32 affects_same_faction_entities; // TODO: make into bitset of ignored factions
    CollisionRetriggerBehaviour retrigger_behaviour;

    union {
        struct {
            Damage damage;
        } deal_damage;
    } as;
} OnCollisionEffect;

typedef struct {
    // TODO: offset from pos
    Vector2 size;

    struct {
	// TODO: there should propably only be one of each kind, so don't use array here
        OnCollisionEffect effects[4];
        s32 count;
    } on_collide_effects;
} ColliderComponent;

static inline OnCollisionEffect *add_collide_effect(ColliderComponent *c, CollideEffectKind kind)
{
    ASSERT(c->on_collide_effects.count < ARRAY_COUNT(c->on_collide_effects.effects));
    ssize index = c->on_collide_effects.count++;
    OnCollisionEffect *result = &c->on_collide_effects.effects[index];
    result->kind = kind;

    return result;
}

static inline OnCollisionEffect *get_collide_effect(ColliderComponent *c, CollideEffectKind kind)
{
    for (s32 i = 0; i < c->on_collide_effects.count; ++i) {
        OnCollisionEffect *current = &c->on_collide_effects.effects[i];

        if (current->kind == kind) {
            return current;
        }
    }

    return 0;
}

static inline OnCollisionEffect *get_or_add_collide_effect(ColliderComponent *c, CollideEffectKind kind)
{
    OnCollisionEffect *result = get_collide_effect(c, kind);

    if (!result) {
        result = add_collide_effect(c, kind);
    }

    return result;
}

static inline void add_blocking_collide_effect(ColliderComponent *c, ObjectKind affected_objects, b32 affect_same_faction)
{
    OnCollisionEffect *effect = add_collide_effect(c, ON_COLLIDE_STOP);
    effect->affects_object_kinds = affected_objects;
    effect->affects_same_faction_entities = affect_same_faction;
}

#endif //COLLIDER_H
