#ifndef COMPONENT_ID_H
#define COMPONENT_ID_H

#define component_flag(type) ES_IMPL_COMP_ENUM_BIT_VALUE(ES_IMPL_COMP_ENUM_NAME(type))
#define ES_IMPL_COMP_ENUM_BIT_VALUE(e) ((u64)1 << (e))

typedef u64 ComponentBitset;

#endif //COMPONENT_ID_H
