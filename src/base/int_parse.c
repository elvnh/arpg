#include "int_parse.h"

#include "string8.h"

// TODO: make sure these are correctly defined for different sizes of long etc
#if defined(__GNUC__)
#    define NP_SAFE_ADD_U64(a, b, result_ptr) !__builtin_uaddl_overflow((a), (b), result_ptr)
#    define NP_SAFE_MUL_U64(a, b, result_ptr) !__builtin_umull_overflow((a), (b), result_ptr)
#    define NP_SAFE_ADD_S64(a, b, result_ptr) !__builtin_saddl_overflow((a), (b), result_ptr)
#    define NP_SAFE_MUL_S64(a, b, result_ptr) !__builtin_smull_overflow((a), (b), result_ptr)
#else
#    error
#endif

static MaybeS64 parse_digit(char c, NumberBase base)
{
    MaybeS64 result = {0};

    // TODO: simplify
    switch (base) {
        case NUM_BASE_DEC: {
            if ((c >= '0') && (c <= '9')) {
                result.value = c - '0';
                result.ok = 1;
            }
        } break;

        case NUM_BASE_HEX: {
            if (c >= 'a') {
                c -= 'a' - 'A';
            }

            if ((c >= '0') && (c <= '9')) {
                result.value = c - '0';
                result.ok = 1;
            } else if ((c >= 'A') && (c <= 'F')) {
                result.value = 10 + c - 'A';
                result.ok = 1;
            }
        } break;

        default: {
            ASSERT(0);
        } break;
    }

    return result;
}

MaybeU64 parse_u64(String str, NumberBase base)
{
    MaybeU64 result = {
        .value = 0,
        .ok = str.length > 0,
    };

    for (ssize i = 0; i < str.length; ++i) {
        MaybeS64 digit = parse_digit(str.data[i], base);

        int multiply_ok = NP_SAFE_MUL_U64(result.value, base, &result.value);
        int add_ok = NP_SAFE_ADD_U64(result.value, (u64)digit.value, &result.value);

        if (!(digit.ok && multiply_ok && add_ok)) {
            result.ok = 0;
            break;
        }
    }

    return result;
}

MaybeS64 parse_s64(String str, NumberBase base)
{
    MaybeS64 result = {
        .value = 0,
        .ok = str.length > 0,
    };

    int32_t sign = 1;

    if ((str.length > 1) && (str.data[0] == '-')) {
        sign = -1;

        ++str.data;
        --str.length;
    }

    for (ssize i = 0; i < str.length; ++i) {
        MaybeS64 digit = parse_digit(str.data[i], base);
        s64 signed_digit = (s64)digit.value * sign;

        int multiply_ok = NP_SAFE_MUL_S64(result.value, base, &result.value);
        int add_ok = NP_SAFE_ADD_S64(result.value, signed_digit, &result.value);

        if (!(digit.ok && multiply_ok && add_ok)) {
            result.ok = 0;
            break;
        }
    }

    return result;
}
