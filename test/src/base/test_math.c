#include "test_macros.h"
#include "base/maths.h"

TEST_CASE(round_to_f32)
{
    REQUIRE(round_to_f32(9.3f, 0.5f) == 9.5f);
    REQUIRE(round_to_f32(9.6f, 0.5f) == 9.5f);
    REQUIRE(round_to_f32(9.2f, 0.5f) == 9.0f);

    REQUIRE(round_to_f32(9.2f, 0.25f) == 9.25f);
    REQUIRE(round_to_f32(9.6f, 0.25f) == 9.5f);

    REQUIRE(round_to_f32(0.0f, 0.25f) == 0.0f);
    REQUIRE(round_to_f32(0.1f, 0.25f) == 0.0f);
    REQUIRE(round_to_f32(0.2f, 0.25f) == 0.25f);

    REQUIRE(round_to_f32(-0.1f, 0.25f) == 0.0f);
    REQUIRE(round_to_f32(-0.2f, 0.25f) == -0.25f);

}

TEST_CASE(fraction)
{
    REQUIRE(fraction(0.5f) == 0.5f);
    REQUIRE(fraction(1.5f) == 0.5f);
    REQUIRE(fraction(-1.5f) == 0.5f);
}

TEST_CASE(round_up_to_nearest_multiple_of)
{
    REQUIRE(round_up_to_nearest_multiple_of(0, 10) == 0);
    REQUIRE(round_up_to_nearest_multiple_of(1, 10) == 10);
    REQUIRE(round_up_to_nearest_multiple_of(8, 10) == 10);
    REQUIRE(round_up_to_nearest_multiple_of(10, 10) == 10);
    REQUIRE(round_up_to_nearest_multiple_of(12, 10) == 20);

    REQUIRE(round_up_to_nearest_multiple_of(12, 20) == 20);
    REQUIRE(round_up_to_nearest_multiple_of(20, 20) == 20);
    REQUIRE(round_up_to_nearest_multiple_of(21, 20) == 40);

    REQUIRE(round_up_to_nearest_multiple_of(2, 20) == 20);
    REQUIRE(round_up_to_nearest_multiple_of(-2, 20) == 0);

    REQUIRE(round_up_to_nearest_multiple_of(-200, 10) == -200);
    REQUIRE(round_up_to_nearest_multiple_of(-201, 200) == -200);
}

TEST_CASE(round_down_to_nearest_multiple_of)
{
    REQUIRE(round_down_to_nearest_multiple_of(0, 10) == 0);
    REQUIRE(round_down_to_nearest_multiple_of(8, 10) == 0);
    REQUIRE(round_down_to_nearest_multiple_of(12, 10) == 10);
    REQUIRE(round_down_to_nearest_multiple_of(12, 20) == 0);
    REQUIRE(round_down_to_nearest_multiple_of(20, 20) == 20);

    REQUIRE(round_down_to_nearest_multiple_of(-1, 20) == -20);
    REQUIRE(round_down_to_nearest_multiple_of(-21, 20) == -40);
    REQUIRE(round_down_to_nearest_multiple_of(-21, 50) == -50);
    REQUIRE(round_down_to_nearest_multiple_of(-50, 50) == -50);
    REQUIRE(round_down_to_nearest_multiple_of(-51, 50) == -100);

}
