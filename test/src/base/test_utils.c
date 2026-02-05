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

    REQUIRE(bit_span(0xF, 0, 2) == 0x3);
    REQUIRE(bit_span(0xFF0FF, 4, 12) == 0xF0F);
    REQUIRE(bit_span(0x0, 0, 64) == 0);
    REQUIRE(bit_span(USIZE_MAX, 0, 64) == USIZE_MAX);
    REQUIRE(bit_span(USIZE_MAX, 0, 63) == (USIZE_MAX >> 1));
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

TEST_CASE(byte_offset)
{
    s32 *ptr = (s32 *)123;
    REQUIRE((s64)byte_offset(ptr, 5) == 128);
    REQUIRE((s64)byte_offset(ptr, -3) == 120);
}

TEST_CASE(type_limits_signed)
{
    REQUIRE(S_SIZE_MAX == LLONG_MAX);
    REQUIRE(S_SIZE_MIN == LLONG_MIN);

    REQUIRE(S64_MAX == LLONG_MAX);
    REQUIRE(S64_MIN == LLONG_MIN);

    REQUIRE(S32_MAX == INT_MAX);
    REQUIRE(S32_MIN == INT_MIN);

    REQUIRE(S16_MAX == SHRT_MAX);
    REQUIRE(S16_MIN == SHRT_MIN);

    REQUIRE(S8_MAX == SCHAR_MAX);
    REQUIRE(S8_MIN == SCHAR_MIN);

}

TEST_CASE(type_limits_unsigned)
{
    REQUIRE(USIZE_MAX == SIZE_MAX);
    REQUIRE(U64_MAX == ULLONG_MAX);
    REQUIRE(U32_MAX == UINT_MAX);
    REQUIRE(U16_MAX == USHRT_MAX);
    REQUIRE(U8_MAX == UCHAR_MAX);
}
