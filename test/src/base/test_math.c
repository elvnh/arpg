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
