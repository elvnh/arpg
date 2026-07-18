#ifndef TEST_MACROS_H
#define TEST_MACROS_H

#include "base/temp_arena.h"

/* Implementation macros */
#define IMPL_CONCAT_(a, b) a##b
#define IMPL_CONCAT(a, b) IMPL_CONCAT_(a, b)

#define IMPL_CASE_FUNCTION_NAME_PREFIX IMPL_TEST_FUNCTION_
#define IMPL_GET_CASE_FUNCTION_NAME(name) IMPL_CONCAT(IMPL_CASE_FUNCTION_NAME_PREFIX, name)

/* User macros */
#define TEST_CASE(name) static void IMPL_GET_CASE_FUNCTION_NAME(name)(void)

#define IMPL_REQUIRE(pass, msg)                 \
    do {                                        \
        if (!(pass)) {                          \
            IMPL_save_failed_assert(__FILE__,   \
                msg, __LINE__);                 \
        } else {                                \
            IMPL_save_passed_assert(__FILE__,   \
                msg, __LINE__);                 \
        }                                       \
    } while (0)

#define REQUIRE(expr) IMPL_REQUIRE((expr), #expr)

#define REQUIRE_STR_EQ(a, b)                                            \
    do {                                                                \
        LinearArena temp = temp_arena_begin();                          \
        String IMPL_str_c = format(&temp, "'"FMT_STR"' == '"FMT_STR"'",  \
            FMT_STR_ARG((a)), FMT_STR_ARG((b)));                        \
        IMPL_str_c = str_null_terminate(IMPL_str_c, la_allocator(&temp)); \
        IMPL_REQUIRE(str_equal(a, b), IMPL_str_c.data);                 \
        temp_arena_end(&temp);                                          \
    } while (0)

#define REQUIRE_STR_NE(a, b)                                            \
    do {                                                                \
        LinearArena temp = temp_arena_begin();                          \
        String IMPL_str_c = format(&temp, "'"FMT_STR"' != '"FMT_STR"'",  \
            FMT_STR_ARG((a)), FMT_STR_ARG((b)));                        \
        IMPL_str_c = str_null_terminate(IMPL_str_c, la_allocator(&temp)); \
        IMPL_REQUIRE(!str_equal(a, b), IMPL_str_c.data);                 \
        temp_arena_end(&temp);                                          \
    } while (0)

#endif //TEST_MACROS_H
