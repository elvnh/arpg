// TODO: proper GCC check
#if __GNUC__

    #define BEGIN_IGNORE_STB_WARNINGS                                      \
        _Pragma("GCC diagnostic push")                                     \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")            \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"")                 \
        _Pragma("GCC diagnostic ignored \"-Wduplicated-branches\"")        \
        _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")            \
        _Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")           \

    #define END_IGNORE_STB_WARNINGS _Pragma("GCC diagnostic pop")

#endif


BEGIN_IGNORE_STB_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
END_IGNORE_STB_WARNINGS
