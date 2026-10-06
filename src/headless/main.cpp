#include "traffic.hpp"
#include "m2.hpp"

#include <exception>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace {
int baseline() {
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
    return 0;
}
int m2() {
    traffic::m2::World world;
    while (world.tick()<1200 && !world.invalid()) world.step();
    if (world.invalid()) {std::cerr << world.diagnostic() << '\n';return 1;}
    bool checkpoint=world.active().size()==6 && world.completed_count()==0;
    for (const auto& car:world.active()) {
        checkpoint &= car.x+world.config().length/2<=world.config().stop_line && car.v<=0.1 && !car.crossing_tick;
        checkpoint &= car.qualified==(car.id==1);
    }
    if (!checkpoint) {
        std::cerr << "M2 pre-release checkpoint failed at tick " << world.tick() << '\n';
        for (const auto& car:world.active()) std::cerr << "ID " << car.id << " x=" << car.x << " v=" << car.v << " qualified=" << car.qualified << '\n';
        return 1;
    }
    std::cout << "Pre-release tick: " << world.tick() << " active: " << world.active().size()
              << " completed: " << world.completed_count() << " valid: yes\n";
    for (const auto& car:world.active())
        std::cout << "Held ID " << car.id << " x=" << car.x << " v=" << car.v
                  << " qualified=" << (car.qualified ? "yes" : "no") << '\n';
    world.request_release();
    while (world.tick()<3600 && !world.complete() && !world.invalid()) world.step();
    if (world.invalid()) {std::cerr << world.diagnostic() << '\n';return 1;}
    if (!world.complete()) {
        std::cerr << "M2 timeout at tick " << world.tick() << '\n';
        for (const auto& car:world.active()) std::cerr << "ID " << car.id << " x=" << car.x << " v=" << car.v << '\n';
        return 1;
    }
    bool events=true;
    std::cout << "Release applied tick: " << *world.release_tick() << '\n';
    for (const auto& record:world.records()) {
        events &= record.qualification_tick && record.crossing_tick && record.completion_tick &&
                  *record.qualification_tick<*record.crossing_tick && *record.crossing_tick<*record.completion_tick;
        std::cout << "ID " << record.id << " qualified " << (record.qualification_tick ? std::to_string(*record.qualification_tick) : "absent")
                  << " crossed " << (record.crossing_tick ? std::to_string(*record.crossing_tick) : "absent")
                  << " completed " << (record.completion_tick ? std::to_string(*record.completion_tick) : "absent") << '\n';
    }
    std::cout << "Final tick: " << world.tick() << " time: " << world.time() << " s active: " << world.active().size()
              << " completed: " << world.completed_count() << " valid: " << (!world.invalid() ? "yes" : "no") << '\n';
    return events && world.completed_count()==6 ? 0 : 1;
}
} // namespace

int main(int argc, char** argv) {
    if (argc!=1 && !(argc==3 && std::string_view(argv[1])=="--scenario" &&
        (std::string_view(argv[2])=="m2" || std::string_view(argv[2])=="ts-001"))) {
        std::cerr << "Usage: traffic_headless [--scenario m2|ts-001]\n";
        return 2;
    }
    try {return argc==3 && std::string_view(argv[2])=="ts-001" ? baseline() : m2();}
    catch (const std::exception& error) {std::cerr << "Headless invalid input: " << error.what() << '\n';return 1;}
}
