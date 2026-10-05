#pragma once

#include <bit>
#include <cstdint>
#include <limits>

namespace traffic::detail {

// Internal binary64 arithmetic. World validates finite, positive inputs first.
static_assert(sizeof(double) == sizeof(std::uint64_t));
static_assert(std::numeric_limits<double>::is_iec559);
static_assert(std::numeric_limits<double>::radix == 2);
static_assert(std::numeric_limits<double>::digits == 53);
static_assert(std::numeric_limits<double>::max_exponent == 1024);
static_assert(std::numeric_limits<double>::min_exponent == -1021);
static_assert(std::numeric_limits<double>::has_denorm == std::denorm_present);

struct Uint128 {
    std::uint64_t high;
    std::uint64_t low;
};

// Full 64 x 64 -> 128 multiplication using four bounded 32-bit products.
constexpr Uint128 multiply(std::uint64_t a, std::uint64_t b) {
    constexpr std::uint64_t mask = 0xffff'ffffULL;
    const auto a0 = a & mask, a1 = a >> 32;
    const auto b0 = b & mask, b1 = b >> 32;
    const auto p00 = a0 * b0;
    const auto p01 = a0 * b1;
    const auto p10 = a1 * b0;
    const auto p11 = a1 * b1;
    const auto middle = (p00 >> 32) + (p01 & mask) + (p10 & mask);
    return {p11 + (p01 >> 32) + (p10 >> 32) + (middle >> 32),
            (p00 & mask) | ((middle & mask) << 32)};
}

struct Binary64 {
    std::uint64_t significand;
    int exponent;
};

constexpr Binary64 decompose(double value) {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    const auto raw_exponent = static_cast<int>((bits >> 52) & 0x7ff);
    const auto fraction = bits & ((1ULL << 52) - 1);
    if (raw_exponent == 0) return {fraction, -1074};
    return {(1ULL << 52) | fraction, raw_exponent - 1023 - 52};
}

constexpr int bit_width(Uint128 value) {
    if (value.high != 0) return 128 - std::countl_zero(value.high);
    return 64 - std::countl_zero(value.low);
}

constexpr bool bit_at(Uint128 value, int index) {
    if (index < 0) return false;
    if (index >= 64) return ((value.high >> (index - 64)) & 1ULL) != 0;
    return ((value.low >> index) & 1ULL) != 0;
}

// Exact sign of tick * speed - 20 * lane_length for positive binary64 inputs.
// Products fit in 117 and 58 bits. Their top binary exponents are within int;
// equal-top products are compared bit by bit, so no wide alignment shift occurs.
constexpr bool has_reached_end(std::uint64_t tick, double speed, double lane_length) {
    if (tick == 0) return false;
    const auto velocity = decompose(speed);
    const auto lane = decompose(lane_length);
    const auto left = multiply(tick, velocity.significand);
    const Uint128 right{0, 20 * lane.significand};
    const int left_bits = bit_width(left);
    const int right_bits = bit_width(right);
    const int left_top = velocity.exponent + left_bits - 1;
    const int right_top = lane.exponent + right_bits - 1;
    if (left_top != right_top) return left_top > right_top;
    const int count = left_bits > right_bits ? left_bits : right_bits;
    for (int offset = 0; offset < count; ++offset) {
        const bool left_bit = bit_at(left, left_bits - 1 - offset);
        const bool right_bit = bit_at(right, right_bits - 1 - offset);
        if (left_bit != right_bit) return left_bit;
    }
    return true;
}

} // namespace traffic::detail
