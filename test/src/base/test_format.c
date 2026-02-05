#include "base/allocator.h"
#include "test_macros.h"
#include "base/format.h"

TEST_CASE(format_s64_to_string)
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator allocator = la_allocator(&arena);

    REQUIRE(str_equal(s64_to_string(0, allocator), str_lit("0")));
    REQUIRE(str_equal(s64_to_string(10, allocator), str_lit("10")));
    REQUIRE(str_equal(s64_to_string(12345, allocator), str_lit("12345")));
    REQUIRE(str_equal(s64_to_string(-1, allocator), str_lit("-1")));
    REQUIRE(str_equal(s64_to_string(-12345, allocator), str_lit("-12345")));

    la_destroy(&arena);
}

TEST_CASE(format_f32_to_string)
{
    LinearArena arena = la_create(default_allocator, KB(1));
    Allocator allocator = la_allocator(&arena);

    REQUIRE(str_equal(f32_to_string(0.0f, 1, allocator), str_lit("0.0")));
    REQUIRE(str_equal(f32_to_string(0.0f, 2, allocator), str_lit("0.00")));
    REQUIRE(str_equal(f32_to_string(0.001f, 2, allocator), str_lit("0.00")));
    REQUIRE(str_equal(f32_to_string(0.001f, 3, allocator), str_lit("0.001")));
    REQUIRE(str_equal(f32_to_string(3.14159f, 5, allocator), str_lit("3.14159")));
    REQUIRE(str_equal(f32_to_string(-3.14159f, 5, allocator), str_lit("-3.14159")));

    la_destroy(&arena);
}
