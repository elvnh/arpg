#include "base/utils.h"
#include "test_macros.h"

TEST_CASE(array_count)
{
    s32 arr[4];
    REQUIRE(ARRAY_COUNT(arr) == 4);
}

TEST_CASE(min)
{
    REQUIRE(MIN(0, 1) == 0);
    REQUIRE(MIN(1, 0) == 0);
    REQUIRE(MIN(-1, 1) == -1);
    REQUIRE(MIN(0, 0) == 0);
}

TEST_CASE(max)
{
    REQUIRE(MAX(0, 1) == 1);
    REQUIRE(MAX(1, 0) == 1);
    REQUIRE(MAX(-1, 1) == 1);
    REQUIRE(MAX(0, 0) == 0);
}

TEST_CASE(clamp)
{
    REQUIRE(CLAMP(0, -1, 1) == 0);
    REQUIRE(CLAMP(-1, -1, 1) == -1);
    REQUIRE(CLAMP(-2, -1, 1) == -1);
    REQUIRE(CLAMP(1, -1, 1) == 1);
    REQUIRE(CLAMP(2, -1, 1) == 1);
}

TEST_CASE(is_pow2)
{
    REQUIRE(is_pow2(0) == false);
    REQUIRE(is_pow2(1) == true);
    REQUIRE(is_pow2(2) == true);
    REQUIRE(is_pow2(3) == false);
    REQUIRE(is_pow2(4) == true);
    REQUIRE(is_pow2(8) == true);

    REQUIRE(is_pow2(-1) == false);
    REQUIRE(is_pow2(-2) ==  false);
}

TEST_CASE(bit_span)
{
    REQUIRE(bit_span(0x1, 0, 1) == 0x1);
    REQUIRE(bit_span(0x1, 1, 1) == 0x0);
    REQUIRE(bit_span(0x1, 1, 8) == 0x0);

    REQUIRE(bit_span(0x3, 0, 3) == 0x3);
    REQUIRE(bit_span(0x3, 1, 3) == 0x1);

    REQUIRE(bit_span(0x10, 1, 1) == 0x0);
    REQUIRE(bit_span(0x10, 0, 5) == 0x10);
    REQUIRE(bit_span(0x10, 4, 1) == 0x1);

    REQUIRE(bit_span(0x33, 0, 1) == 0x1);
    REQUIRE(bit_span(0x33, 3, 3) == 0x6);
}

TEST_CASE(multiply_overflows_ssize)
{
    REQUIRE(multiply_overflows_ssize(1, 100) == false);
    REQUIRE(multiply_overflows_ssize(S_SIZE_MAX / 2, 2) == false);
    REQUIRE(multiply_overflows_ssize(S_SIZE_MAX / 2 + 1, 2) == true);
    REQUIRE(multiply_overflows_ssize(S_SIZE_MAX / 3 + 1, 3) == true);
}

TEST_CASE(add_overflows_ssize)
{
    REQUIRE(add_overflows_ssize(0, 1) == false);
    REQUIRE(add_overflows_ssize(S_SIZE_MAX, 0) == false);
    REQUIRE(add_overflows_ssize(S_SIZE_MAX, 1) == true);
    REQUIRE(add_overflows_ssize(S_SIZE_MAX - 1, 1) == false);
    REQUIRE(add_overflows_ssize(S_SIZE_MAX - 1, 2) == true);
    REQUIRE(add_overflows_ssize(0, S_SIZE_MAX) == false);
    REQUIRE(add_overflows_ssize(1, S_SIZE_MAX) == true);
}

TEST_CASE(is_aligned)
{
    REQUIRE(is_aligned(0, 8) == true);
    REQUIRE(is_aligned(1, 8) == false);
    REQUIRE(is_aligned(3, 1) == true);

    REQUIRE(is_aligned(8, 8) == true);
    REQUIRE(is_aligned(16, 8) == true);
    REQUIRE(is_aligned(17, 8) == false);
}

TEST_CASE(align)
{
    REQUIRE(align(0, 4) == 0);
    REQUIRE(align(1, 4) == 4);
    REQUIRE(align(3, 4) == 4);
    REQUIRE(align(4, 4) == 4);
    REQUIRE(align(5, 4) == 8);
}
