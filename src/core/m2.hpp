#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace traffic::m2 {

struct Config {
    double lane_start = 0.0, lane_end = 600.0, stop_line = 400.0;
    double length = 4.5, width = 1.8;
    double desired_speed = 65.0 / 3.6, headway = 1.5, standstill_gap = 2.0;
    double acceleration = 1.5, comfortable_braking = 2.0;
    int exponent = 4;
    double virtual_obstacle = 401.5;
};

struct Vehicle {
    int id = 0;
    double x = 0.0, v = 0.0;
    int dwell = 0;
    bool qualified = false;
    std::optional<std::uint64_t> qualification_tick, crossing_tick, completion_tick;
};

struct Motion {
    double x = 0.0, v = 0.0, acceleration = 0.0, moving_until = 0.05;
    [[nodiscard]] double position(double t) const;
    [[nodiscard]] double speed(double t) const;
};

struct Record {
    int id = 0;
    std::optional<std::uint64_t> qualification_tick, crossing_tick, completion_tick;
    double completion_x = 0.0, completion_v = 0.0;
};

// Pure model components. Invalid inputs throw; the World catches and latches diagnostics.
double idm(const Config& config, double speed, std::optional<double> gap = {}, double leader_speed = 0.0);
Motion ballistic(double x, double v, double acceleration, double dt = 0.05);
void validate_trajectory(const Motion& leader, const Motion& follower, double length, double dt = 0.05);
void validate_line(const Motion& motion, double length, double line, double dt = 0.05);
bool full_stop_interval(const Motion& motion, double length, bool eligible, double dt = 0.05);

class World {
public:
    static constexpr double step_seconds = 0.05;
    World();
    explicit World(Config config, std::vector<Vehicle> vehicles);
    void reset();
    void request_release();
    void step();
    [[nodiscard]] const Config& config() const { return config_; }
    [[nodiscard]] const std::vector<Vehicle>& active() const { return active_; }
    [[nodiscard]] const std::vector<Record>& records() const { return records_; }
    [[nodiscard]] std::uint64_t tick() const { return tick_; }
    [[nodiscard]] double time() const { return tick_ * step_seconds; }
    [[nodiscard]] bool released() const { return released_; }
    [[nodiscard]] bool release_pending() const { return release_pending_; }
    [[nodiscard]] bool complete() const { return active_.empty() && !invalid_; }
    [[nodiscard]] bool invalid() const { return invalid_; }
    [[nodiscard]] const std::string& diagnostic() const { return diagnostic_; }
    [[nodiscard]] std::optional<std::uint64_t> release_tick() const { return release_tick_; }
    [[nodiscard]] int completed_count() const;
private:
    Config config_;
    std::vector<Vehicle> initial_, active_;
    std::vector<Record> records_;
    std::uint64_t tick_ = 0;
    bool released_ = false, release_pending_ = false, invalid_ = false;
    std::optional<std::uint64_t> release_tick_;
    std::string diagnostic_;
};

class Driver {
public:
    static constexpr std::int64_t step_ns = 50'000'000;
    Driver() = default;
    void reset();
    void request_release() { world_.request_release(); }
    void schedule_release(std::uint64_t tick);
    void set_running(bool running) { running_ = running && !world_.complete() && !world_.invalid(); }
    void set_playback(int multiplier);
    void advance(std::int64_t elapsed_ns, std::uint32_t max_steps = 1000);
    [[nodiscard]] const World& world() const { return world_; }
    [[nodiscard]] bool running() const { return running_; }
    [[nodiscard]] int playback() const { return playback_; }
    [[nodiscard]] std::int64_t backlog_ns() const { return backlog_ns_; }
    [[nodiscard]] double display_x(int id) const;
private:
    World world_;
    std::vector<Vehicle> previous_;
    bool running_ = false;
    int playback_ = 1;
    std::int64_t backlog_ns_ = 0;
    std::optional<std::uint64_t> scheduled_release_tick_;
};

} // namespace traffic::m2
