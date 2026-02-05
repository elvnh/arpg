#ifndef TEST_MACROS_H
#define TEST_MACROS_H

/* Implementation macros */
#define IMPL_CONCAT_(a, b) a##b
#define IMPL_CONCAT(a, b) IMPL_CONCAT_(a, b)

#define IMPL_CASE_FUNCTION_NAME_PREFIX IMPL_TEST_FUNCTION_
#define IMPL_GET_CASE_FUNCTION_NAME(name) IMPL_CONCAT(IMPL_CASE_FUNCTION_NAME_PREFIX, name)

/* User macros */
#define TEST_CASE(name) static void IMPL_GET_CASE_FUNCTION_NAME(name)(void)

#define REQUIRE(expr)                                                   \
    do {                                                                \
        if (!(expr)) {                                                  \
            IMPL_save_failed_assert(__FILE__,                           \
                #expr, __LINE__);                                       \
        } else {                                                        \
            IMPL_save_passed_assert(__FILE__,                           \
                #expr, __LINE__);                                       \
        }                                                               \
    } while (0)

#endif //TEST_MACROS_H
