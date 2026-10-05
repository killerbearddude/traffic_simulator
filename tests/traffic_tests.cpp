#include "traffic.hpp"
#include "detail/exact_arrival.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <vector>

using traffic::Driver;
using traffic::World;

TEST_CASE("initial state and one fixed step") {
    World world;
    REQUIRE(world.tick() == 0);
    REQUIRE(world.distance() == 0.0);
    REQUIRE(world.active_count() == 1);
    world.step();
    REQUIRE(world.tick() == 1);
    REQUIRE(world.distance() == 0.5);
    REQUIRE(world.time() == 0.05);
}

TEST_CASE("baseline completes once and stops its clock") {
    World world;
    for (int i = 0; i < 199; ++i) world.step();
    REQUIRE_FALSE(world.complete());
    world.step();
    REQUIRE(world.complete());
    REQUIRE(world.tick() == 200);
    REQUIRE(world.time() == 10.0);
    REQUIRE(world.distance() == 100.0);
    REQUIRE(world.active_count() == 0);
    REQUIRE(world.completed_count() == 1);
    REQUIRE(world.completion().tick == 200);
    REQUIRE(world.completion().time == 10.0);
    for (int i = 0; i < 100; ++i) world.step();
    REQUIRE(world.tick() == 200);
    REQUIRE(world.completed_count() == 1);
}

TEST_CASE("off-grid completion records first completing tick") {
    World world({100.25, 10.0});
    for (int i = 0; i < 201; ++i) world.step();
    REQUIRE(world.complete());
    REQUIRE(world.completion().tick == 201);
    REQUIRE(world.completion().time == 10.05);
    REQUIRE(world.completion().distance == 100.25);
}

TEST_CASE("constant-speed fixtures complete on the intended tick") {
    struct Case { double length; double speed; std::uint64_t completion_tick; };
    const Case cases[] = {
        {100.0, 10.0, 200}, {100.25, 10.0, 201},
        {1.0, 2.0, 10}, {100.0, 2.0, 1000}, {100.0, 1.0, 2000},
        {1.0 - 1e-9, 2.0, 10}, {1.0 + 1e-9, 2.0, 11},
        {std::nextafter(1.0, 0.0), 2.0, 10},
        {std::nextafter(1.0, std::numeric_limits<double>::infinity()), 2.0, 11},
    };
    for (const auto& item : cases) {
        INFO("length=" << item.length << ", speed=" << item.speed);
        World world({item.length, item.speed});
        for (std::uint64_t i = 1; i < item.completion_tick; ++i) {
            world.step();
            REQUIRE_FALSE(world.complete());
            REQUIRE(world.active_count() == 1);
            REQUIRE(world.completed_count() == 0);
            REQUIRE(world.distance() < item.length);
        }
        world.step();
        REQUIRE(world.complete());
        REQUIRE(world.tick() == item.completion_tick);
        REQUIRE(world.time() == static_cast<double>(item.completion_tick) * World::step_seconds);
        REQUIRE(world.distance() == item.length);
        REQUIRE(world.completion().tick == item.completion_tick);
        REQUIRE(world.completion().time == world.time());
        REQUIRE(world.completion().distance == item.length);
        REQUIRE(world.active_count() == 0);
        REQUIRE(world.completed_count() == 1);
        for (int i = 0; i < 5; ++i) world.step();
        REQUIRE(world.tick() == item.completion_tick);
        REQUIRE(world.completed_count() == 1);
        REQUIRE(world.completion().tick == item.completion_tick);
    }
}

TEST_CASE("binary64 speeds near arrival boundaries do not complete early") {
    struct Case { double length; double speed; std::uint64_t completion_tick; };
    const Case cases[] = {
        {100.0, 0x1.993b5021f75adp-3, 10010},
        {100.0, 0x1.993b5021f75aep-3, 10010},
        {100.0, 0x1.993b5021f75afp-3, 10009},
        {1000.0, 0x1.8de2dfee2cb8ep+2, 3218},
        {1000.0, 0x1.8de2dfee2cb8fp+2, 3218},
        {1000.0, 0x1.8de2dfee2cb90p+2, 3217},
    };
    for (const auto& item : cases) {
        INFO("length=" << item.length << ", speed=" << std::hexfloat << item.speed);
        World world({item.length, item.speed});
        for (std::uint64_t i = 1; i < item.completion_tick; ++i) {
            const double previous_distance = world.distance();
            world.step();
            REQUIRE_FALSE(world.complete());
            REQUIRE(world.active_count() == 1);
            REQUIRE(world.completed_count() == 0);
            REQUIRE(world.distance() >= 0.0);
            REQUIRE(std::isfinite(world.distance()));
            REQUIRE(world.distance() >= previous_distance);
            REQUIRE(world.distance() < item.length);
        }
        world.step();
        REQUIRE(world.complete());
        REQUIRE(world.tick() == item.completion_tick);
        REQUIRE(world.distance() == item.length);
        REQUIRE(world.time() == static_cast<double>(item.completion_tick) * World::step_seconds);
        REQUIRE(world.completion().tick == item.completion_tick);
        REQUIRE(world.completion().time == world.time());
        REQUIRE(world.completion().distance == item.length);
        REQUIRE(world.active_count() == 0);
        REQUIRE(world.completed_count() == 1);
        for (int i = 0; i < 5; ++i) world.step();
        REQUIRE(world.tick() == item.completion_tick);
        REQUIRE(world.completed_count() == 1);
        REQUIRE(world.completion().tick == item.completion_tick);
    }
}

TEST_CASE("exact arrival arithmetic covers carries, exponent gaps, and large ticks") {
    constexpr auto square = traffic::detail::multiply(
        std::numeric_limits<std::uint64_t>::max(), std::numeric_limits<std::uint64_t>::max());
    STATIC_REQUIRE(square.high == std::numeric_limits<std::uint64_t>::max() - 1);
    STATIC_REQUIRE(square.low == 1);

    const double smallest = std::numeric_limits<double>::denorm_min();
    const double largest = std::numeric_limits<double>::max();
    for (double value : {smallest, largest}) {
        REQUIRE_FALSE(traffic::detail::has_reached_end(19, value, value));
        REQUIRE(traffic::detail::has_reached_end(20, value, value));
        World world({value, value});
        for (int i = 0; i < 19; ++i) world.step();
        REQUIRE_FALSE(world.complete());
        world.step();
        REQUIRE(world.complete());
        REQUIRE(world.tick() == 20);
        REQUIRE(world.distance() == value);
        REQUIRE(world.completion().distance == value);
    }

    const std::uint64_t carry_tick = 1ULL << 63;
    const double carry_lane = 0x1p62;
    REQUIRE_FALSE(traffic::detail::has_reached_end(carry_tick, std::nextafter(10.0, 0.0), carry_lane));
    REQUIRE(traffic::detail::has_reached_end(carry_tick, 10.0, carry_lane));
    REQUIRE(traffic::detail::has_reached_end(carry_tick,
        std::nextafter(10.0, std::numeric_limits<double>::infinity()), carry_lane));

    const auto last_tick = std::numeric_limits<std::uint64_t>::max();
    REQUIRE_FALSE(traffic::detail::has_reached_end(0, 20.0, 0x1p64));
    REQUIRE_FALSE(traffic::detail::has_reached_end(last_tick, 20.0, 0x1p64));
    REQUIRE(traffic::detail::has_reached_end(last_tick,
        std::nextafter(20.0, std::numeric_limits<double>::infinity()), 0x1p64));
    REQUIRE_FALSE(traffic::detail::has_reached_end(last_tick, smallest, 1.0));
    REQUIRE(traffic::detail::has_reached_end(1, largest, smallest));
}

TEST_CASE("invalid fixtures are rejected") {
    for (double bad : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        REQUIRE_THROWS_AS(World({bad, 10.0}), std::invalid_argument);
        REQUIRE_THROWS_AS(World({100.0, bad}), std::invalid_argument);
    }
}

TEST_CASE("driver pause preserves remainder and excludes paused elapsed time") {
    Driver driver;
    driver.set_running(true);
    driver.advance(75'000'000);
    REQUIRE(driver.world().tick() == 1);
    REQUIRE(driver.backlog_ns() == 25'000'000);
    driver.set_running(false);
    driver.advance(5'000'000'000);
    REQUIRE(driver.world().tick() == 1);
    REQUIRE(driver.backlog_ns() == 25'000'000);
    driver.set_running(true);
    driver.advance(25'000'000);
    REQUIRE(driver.world().tick() == 2);
    REQUIRE(driver.backlog_ns() == 0);
}

TEST_CASE("playback changes step count, not step size") {
    Driver driver;
    driver.set_running(true);
    driver.advance(50'000'000);
    driver.set_playback(2);
    driver.advance(50'000'000);
    driver.set_playback(4);
    driver.advance(50'000'000);
    REQUIRE(driver.world().tick() == 7);
    REQUIRE(driver.world().distance() == 3.5);
    REQUIRE_THROWS_AS(driver.set_playback(3), std::invalid_argument);
    REQUIRE_THROWS_AS(driver.advance(-1), std::invalid_argument);
}

TEST_CASE("reset clears clock, history, completion and playback") {
    Driver driver;
    driver.set_playback(4);
    driver.set_running(true);
    driver.advance(3'000'000'000);
    REQUIRE(driver.world().complete());
    driver.reset();
    REQUIRE_FALSE(driver.running());
    REQUIRE(driver.playback() == 1);
    REQUIRE(driver.world().tick() == 0);
    REQUIRE(driver.world().completed_count() == 0);
    REQUIRE(driver.backlog_ns() == 0);
    REQUIRE(driver.previous_distance() == 0.0);
    REQUIRE(driver.display_distance() == 0.0);
}

TEST_CASE("long frame retains backlog and eventually completes") {
    Driver driver;
    driver.set_running(true);
    driver.advance(10'000'000'000, 3);
    REQUIRE(driver.world().tick() == 3);
    REQUIRE(driver.backlog_ns() == 9'850'000'000);
    while (driver.running()) driver.advance(0, 7);
    REQUIRE(driver.world().tick() == 200);
    REQUIRE(driver.backlog_ns() == 0);
    REQUIRE(driver.world().completed_count() == 1);
}

TEST_CASE("interpolation is presentation only") {
    Driver driver;
    driver.set_running(true);
    driver.advance(75'000'000);
    const auto tick = driver.world().tick();
    const auto distance = driver.world().distance();
    REQUIRE(driver.display_distance() == 0.25);
    REQUIRE(driver.world().tick() == tick);
    REQUIRE(driver.world().distance() == distance);
}

namespace {
std::vector<double> run_schedule(int frames, int playback, bool irregular = false,
                                 traffic::Fixture fixture = {}, std::uint64_t completion_tick = 200) {
    Driver driver(fixture);
    driver.set_playback(playback);
    driver.set_running(true);
    std::vector<double> distances;
    const std::int64_t total_ns = static_cast<std::int64_t>(completion_tick) * Driver::step_ns / playback;
    std::int64_t previous_boundary = 0;
    for (int frame = 1; frame <= frames; ++frame) {
        const auto boundary = irregular
            ? total_ns * frame * frame / (frames * frames)
            : total_ns * frame / frames;
        const auto old_tick = driver.world().tick();
        driver.advance(boundary - previous_boundary, 1);
        if (driver.world().tick() != old_tick) distances.push_back(driver.world().distance());
        previous_boundary = boundary;
        while (driver.running() && driver.backlog_ns() >= Driver::step_ns) {
            driver.advance(0, 1);
            distances.push_back(driver.world().distance());
        }
    }
    REQUIRE(driver.world().complete());
    REQUIRE(driver.world().completion().tick == completion_tick);
    REQUIRE(driver.world().completion().time == static_cast<double>(completion_tick) * World::step_seconds);
    return distances;
}
} // namespace

TEST_CASE("formerly late fixture keeps its trajectory across frame schedules") {
    World reference({1.0, 2.0});
    std::vector<double> expected;
    while (!reference.complete()) {
        reference.step();
        expected.push_back(reference.distance());
    }
    REQUIRE(reference.completion().tick == 10);
    for (int playback : {1, 2, 4}) {
        for (int frames : {1, 30 / playback, 17}) {
            REQUIRE(run_schedule(frames, playback, frames == 17, {1.0, 2.0}, 10) == expected);
        }
    }
}

TEST_CASE("early-arrival regression keeps tick 10010 across frame schedules") {
    constexpr traffic::Fixture fixture{100.0, 0x1.993b5021f75aep-3};
    constexpr std::uint64_t expected_tick = 10010; // Exact rational oracle, not World output.
    World reference(fixture);
    std::vector<double> expected_states;
    for (std::uint64_t i = 0; i < expected_tick; ++i) {
        reference.step();
        expected_states.push_back(reference.distance());
    }
    REQUIRE(reference.completion().tick == expected_tick);
    for (int playback : {1, 2, 4}) {
        for (int frames : {1, 15015 / playback, 30030 / playback, 72072 / playback, 173}) {
            const auto states = run_schedule(frames, playback, frames == 173, fixture, expected_tick);
            REQUIRE(states.size() == expected_tick);
            REQUIRE(states == expected_states);
        }
    }
}

TEST_CASE("regular and irregular frame schedules preserve trajectory") {
    for (int playback : {1, 2, 4}) {
        for (int frames : {1, 30 * 10 / playback, 60 * 10 / playback, 144 * 10 / playback, 173}) {
            const auto states = run_schedule(frames, playback, frames == 173);
            REQUIRE(states.size() == 200);
            for (std::size_t i = 0; i < states.size(); ++i) {
                REQUIRE(states[i] == static_cast<double>(i + 1) * 0.5);
            }
        }
    }
}
