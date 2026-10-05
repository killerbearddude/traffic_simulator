#pragma once

#include <cstdint>

namespace traffic::app {

enum class PacingMode { VSync, Timed };

constexpr std::uint64_t target_frame_ns = 1'000'000'000ULL / 60;

constexpr PacingMode select_pacing(bool force_fallback, bool vsync_request_succeeded) {
    return !force_fallback && vsync_request_succeeded ? PacingMode::VSync : PacingMode::Timed;
}

// Call after presentation with the same monotonic clock used at frame start.
// A late frame schedules no compensating renders and never underflows the wait.
template <typename Wait>
void finish_frame(PacingMode mode, std::uint64_t frame_start_ns, std::uint64_t now_ns, Wait&& wait) {
    if (mode != PacingMode::Timed || now_ns < frame_start_ns) return;
    const auto elapsed_ns = now_ns - frame_start_ns;
    if (elapsed_ns < target_frame_ns) wait(target_frame_ns - elapsed_ns);
}

} // namespace traffic::app
