#include "traffic.hpp"

#include <exception>
#include <iomanip>
#include <iostream>

int main() {
    try {
        traffic::World world;
        for (int i = 0; i < 10000 && !world.complete(); ++i) world.step();
        if (!world.complete() || world.tick() != 200 || world.completed_count() != 1) {
            std::cerr << "Baseline did not complete as expected\n";
            return 1;
        }
        const auto fixture = world.fixture();
        const auto record = world.completion();
        std::cout << std::fixed << std::setprecision(2)
                  << "Lane length: " << fixture.lane_length << " m\n"
                  << "Vehicle speed: " << fixture.speed << " m/s\n"
                  << "Fixed step: " << traffic::World::step_seconds << " s\n"
                  << "Completion tick: " << record.tick << "\n"
                  << "Completion time: " << record.time << " s\n"
                  << "Final distance: " << record.distance << " m\n"
                  << "Active: " << world.active_count() << "\n"
                  << "Completed: " << world.completed_count() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Headless run failed: " << error.what() << '\n';
        return 1;
    }
}
