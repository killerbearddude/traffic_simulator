#include "traffic.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
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
std::vector<double> run_schedule(int frames, int playback, bool irregular = false) {
    Driver driver;
    driver.set_playback(playback);
    driver.set_running(true);
    std::vector<double> distances;
    const std::int64_t total_ns = 10'000'000'000LL / playback;
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
    REQUIRE(driver.world().completion().tick == 200);
    REQUIRE(driver.world().completion().time == 10.0);
    return distances;
}
} // namespace

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
