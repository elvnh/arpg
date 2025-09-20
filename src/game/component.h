#ifndef COMPONENT_H
#define COMPONENT_H

#define COMPONENT_LIST                          \
    COMPONENT(PhysicsComponent)                 \
    COMPONENT(ColliderComponent)                \

#define ES_IMPL_COMP_ENUM_NAME(type) COMP_##type
#define ES_IMPL_COMP_FIELD_NAME(type) component_##type
#define ES_IMPL_COMP_ENUM_BIT_VALUE(e) ((u64)1 << (e))

#define component_flag(type) ES_IMPL_COMP_ENUM_BIT_VALUE(ES_IMPL_COMP_ENUM_NAME(type))

typedef u64 ComponentBitset;

typedef enum {
    #define COMPONENT(type) ES_IMPL_COMP_ENUM_NAME(type),
        COMPONENT_LIST
        COMPONENT_COUNT,
    #undef COMPONENT
} ComponentType;

typedef struct {
    Vector2 position;
    Vector2 velocity;
} PhysicsComponent;

typedef struct {
    Vector2 size;
} ColliderComponent;



#endif //COMPONENT_H
