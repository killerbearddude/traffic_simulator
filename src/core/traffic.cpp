#include "traffic.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace traffic {

namespace {
void validate(Fixture fixture) {
    if (!std::isfinite(fixture.lane_length) || fixture.lane_length <= 0.0 ||
        !std::isfinite(fixture.speed) || fixture.speed <= 0.0) {
        throw std::invalid_argument("lane length and speed must be finite and positive");
    }
}
} // namespace

World::World(Fixture fixture) { reset(fixture); }

void World::reset(Fixture fixture) {
    validate(fixture);
    fixture_ = fixture;
    tick_ = 0;
    distance_ = 0.0;
    complete_ = false;
    completion_ = {};
}

void World::step() {
    if (complete_) return;
    ++tick_;
    // This constant-speed milestone uses the exact fixed-step ratio 1/20 s.
    // Deriving travel from the integer tick avoids accumulated addition error.
    const long double traveled = static_cast<long double>(tick_) * fixture_.speed / 20.0L;
    if (traveled >= static_cast<long double>(fixture_.lane_length)) {
        distance_ = fixture_.lane_length;
        complete_ = true;
        completion_ = {tick_, time(), distance_};
    } else {
        distance_ = static_cast<double>(traveled);
        if (distance_ >= fixture_.lane_length) {
            distance_ = std::nextafter(fixture_.lane_length, 0.0);
        }
    }
}

Driver::Driver(Fixture fixture) : world_(fixture) {}

void Driver::reset(Fixture fixture) {
    world_.reset(fixture);
    running_ = false;
    playback_ = 1;
    backlog_ns_ = 0;
    previous_distance_ = 0.0;
}

void Driver::set_playback(int multiplier) {
    if (multiplier != 1 && multiplier != 2 && multiplier != 4) {
        throw std::invalid_argument("playback must be 1, 2, or 4");
    }
    playback_ = multiplier;
}

void Driver::advance(std::int64_t elapsed_ns, std::uint32_t max_steps) {
    if (elapsed_ns < 0) throw std::invalid_argument("elapsed time must be nonnegative");
    if (!running_ || world_.complete()) return;
    if (elapsed_ns > (std::numeric_limits<std::int64_t>::max() - backlog_ns_) / playback_) {
        throw std::overflow_error("elapsed time exceeds driver capacity");
    }
    backlog_ns_ += elapsed_ns * playback_;
    for (std::uint32_t i = 0; i < max_steps && backlog_ns_ >= step_ns && !world_.complete(); ++i) {
        previous_distance_ = world_.distance();
        world_.step();
        backlog_ns_ -= step_ns;
    }
    if (world_.complete()) {
        running_ = false;
        backlog_ns_ = 0;
    }
}

double Driver::display_distance() const {
    if (world_.complete()) return world_.distance();
    const double alpha = std::clamp(static_cast<double>(backlog_ns_) / step_ns, 0.0, 1.0);
    return previous_distance_ + (world_.distance() - previous_distance_) * alpha;
}

} // namespace traffic
