#pragma once

#include <cstdint>

namespace traffic {

// Distances are meters, speeds are meters/second, and time is seconds.
struct Fixture {
    double lane_length = 100.0;
    double speed = 10.0;
};

struct Completion {
    std::uint64_t tick = 0;
    double time = 0.0;
    double distance = 0.0;
};

class World {
public:
    static constexpr double step_seconds = 0.05;
    explicit World(Fixture fixture = {});
    void reset(Fixture fixture = {});
    void step();

    [[nodiscard]] Fixture fixture() const { return fixture_; }
    [[nodiscard]] std::uint64_t tick() const { return tick_; }
    [[nodiscard]] double time() const { return static_cast<double>(tick_) * step_seconds; }
    [[nodiscard]] double distance() const { return distance_; }
    [[nodiscard]] bool complete() const { return complete_; }
    [[nodiscard]] int active_count() const { return complete_ ? 0 : 1; }
    [[nodiscard]] int completed_count() const { return complete_ ? 1 : 0; }
    [[nodiscard]] Completion completion() const { return completion_; }

private:
    Fixture fixture_;
    std::uint64_t tick_ = 0;
    double distance_ = 0.0;
    bool complete_ = false;
    Completion completion_;
};

// Caller supplies active wall time in integer nanoseconds; paused time is ignored.
// A processing budget defers excess work without dropping it.
class Driver {
public:
    static constexpr std::int64_t step_ns = 50'000'000;
    explicit Driver(Fixture fixture = {});
    void reset(Fixture fixture = {});
    void set_running(bool running) { running_ = running && !world_.complete(); }
    void set_playback(int multiplier);
    void advance(std::int64_t elapsed_ns, std::uint32_t max_steps = 1000);

    [[nodiscard]] const World& world() const { return world_; }
    [[nodiscard]] bool running() const { return running_; }
    [[nodiscard]] int playback() const { return playback_; }
    [[nodiscard]] std::int64_t backlog_ns() const { return backlog_ns_; }
    [[nodiscard]] double previous_distance() const { return previous_distance_; }
    // Presentation only. When behind schedule, show the latest completed state.
    [[nodiscard]] double display_distance() const;

private:
    World world_;
    bool running_ = false;
    int playback_ = 1;
    std::int64_t backlog_ns_ = 0;
    double previous_distance_ = 0.0;
};

} // namespace traffic
