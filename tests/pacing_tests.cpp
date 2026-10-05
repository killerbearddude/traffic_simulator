#include "frame_pacing.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

using traffic::app::PacingMode;
using traffic::app::finish_frame;
using traffic::app::select_pacing;
using traffic::app::target_frame_ns;

TEST_CASE("pacing selection handles VSync success, failure, and forced fallback") {
    REQUIRE(select_pacing(false, true) == PacingMode::VSync);
    REQUIRE(select_pacing(false, false) == PacingMode::Timed);
    REQUIRE(select_pacing(true, true) == PacingMode::Timed);
    REQUIRE(select_pacing(true, false) == PacingMode::Timed);
}

TEST_CASE("timed pacing waits only for the remaining frame budget") {
    std::uint64_t requested_ns = 0;
    int calls = 0;
    const auto wait = [&](std::uint64_t duration) { requested_ns = duration; ++calls; };
    finish_frame(PacingMode::Timed, 100, 100 + 5'000'000, wait);
    REQUIRE(calls == 1);
    REQUIRE(requested_ns == target_frame_ns - 5'000'000);
    finish_frame(PacingMode::Timed, 100, 100 + target_frame_ns, wait);
    finish_frame(PacingMode::Timed, 100, 100 + target_frame_ns + 10'000'000, wait);
    finish_frame(PacingMode::Timed, 100, 99, wait);
    REQUIRE(calls == 1);
}

TEST_CASE("VSync mode never adds a fallback wait") {
    int calls = 0;
    finish_frame(PacingMode::VSync, 100, 101, [&](std::uint64_t) { ++calls; });
    REQUIRE(calls == 0);
}
