#ifndef COLLISION_H
#define COLLISION_H

#include "base/vector.h"
#include "base/rectangle.h"
#include "base/linear_arena.h"
#include "entity.h"

typedef struct {
    Vector2  new_position_a;
    Vector2  new_position_b;

    Vector2  new_velocity_a;
    Vector2  new_velocity_b;

    f32      movement_fraction_left;

    Vector2 collision_normal;
    b32 are_colliding;
} CollisionInfo;


// TODO: more arbitrary collision responses
typedef struct CollisionRule {
    struct CollisionRule *next;
    struct CollisionRule *prev;
    EntityPair		  entity_pair;
    b32			  should_collide;
} CollisionRule;

typedef struct {
    CollisionRule *head;
    CollisionRule *tail;
} CollisionRuleList;

typedef struct {
    CollisionRuleList  table[512];
    CollisionRuleList  free_node_list;
} CollisionRuleTable;

CollisionInfo collision_rect_vs_rect(f32 movement_fraction_left, Rectangle rect_a, Rectangle rect_b,
    Vector2 velocity_a, Vector2 velocity_b, f32 dt);
CollisionRule *collision_rule_find(CollisionRuleTable *table, EntityID a, EntityID b);
void collision_rule_add(CollisionRuleTable *table, EntityID a, EntityID b, b32 should_collide,
    LinearArena *arena);
void remove_collision_rules_with_entity(CollisionRuleTable *table, EntityID a);

#endif //COLLISION_H
