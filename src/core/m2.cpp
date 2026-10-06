#include "m2.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace traffic::m2 {
namespace {
void finite(double value, const char* field) {
    if (!std::isfinite(value)) throw std::invalid_argument(std::string("nonfinite ") + field);
}
void validate_motion(const Motion& m, double dt) {
    finite(m.x,"trajectory position"); finite(m.v,"trajectory speed");
    finite(m.acceleration,"trajectory acceleration"); finite(m.moving_until,"trajectory stopping time");
    if (m.v < 0 || m.moving_until < 0 || m.moving_until > dt ||
        !std::isfinite(m.position(dt)) || !std::isfinite(m.speed(dt)) ||
        m.position(dt) < m.x || m.speed(dt) < 0)
        throw std::runtime_error("invalid or backwards trajectory");
}
void validate_config(const Config& c, const std::vector<Vehicle>& cars) {
    for (const auto [value, name] : {std::pair{c.lane_start,"lane_start"}, {c.lane_end,"lane_end"},
            {c.stop_line,"stop_line"}, {c.length,"length"}, {c.width,"width"},
            {c.desired_speed,"desired_speed"}, {c.headway,"headway"},
            {c.standstill_gap,"standstill_gap"}, {c.acceleration,"acceleration"},
            {c.comfortable_braking,"comfortable_braking"}, {c.virtual_obstacle,"virtual_obstacle"}}) finite(value, name);
    if (!(c.lane_start < c.stop_line && c.stop_line < c.lane_end) ||
        !(c.length > 0 && c.width > 0 && c.desired_speed > 0 && c.headway > 0 &&
          c.standstill_gap >= 0 && c.acceleration > 0 && c.comfortable_braking > 0 && c.exponent == 4 &&
          c.virtual_obstacle > c.stop_line)) throw std::invalid_argument("invalid M2 configuration geometry or IDM parameter");
    if (cars.empty()) throw std::invalid_argument("empty M2 fixture");
    for (std::size_t i = 0; i < cars.size(); ++i) {
        const auto& car = cars[i];
        if (car.id <= 0) throw std::invalid_argument("invalid vehicle ID");
        finite(car.x, "vehicle position"); finite(car.v, "vehicle speed");
        if (car.v < 0 || car.x - c.length/2 < c.lane_start || car.x + c.length/2 >= c.stop_line ||
            car.dwell != 0 || car.qualified || car.qualification_tick || car.crossing_tick || car.completion_tick)
            throw std::invalid_argument("invalid initial vehicle state: ID " + std::to_string(car.id));
        for (std::size_t j = 0; j < i; ++j)
            if (cars[j].id == car.id) throw std::invalid_argument("duplicate vehicle ID " + std::to_string(car.id));
        if (i && !(cars[i-1].x - car.x > c.length))
            throw std::invalid_argument("overlapping or unordered initial vehicles: ID " + std::to_string(car.id));
    }
}
std::vector<Vehicle> canonical() {
    std::vector<Vehicle> cars;
    for (int i = 0; i < 6; ++i) cars.push_back({i+1, 240.0 - 40.0*i, 65.0/3.6});
    return cars;
}
} // namespace

double idm(const Config& c, double v, std::optional<double> gap, double leader_speed) {
    finite(v, "IDM speed"); finite(leader_speed, "IDM leader speed");
    if (v < 0 || leader_speed < 0) throw std::invalid_argument("negative IDM speed");
    if (!(std::isfinite(c.desired_speed) && c.desired_speed > 0 && std::isfinite(c.acceleration) && c.acceleration > 0 &&
          std::isfinite(c.comfortable_braking) && c.comfortable_braking > 0 && std::isfinite(c.headway) && c.headway > 0 &&
          std::isfinite(c.standstill_gap) && c.standstill_gap >= 0 && c.exponent == 4))
        throw std::invalid_argument("invalid IDM configuration");
    const double ratio = v / c.desired_speed;
    const double free = c.acceleration * (1 - std::pow(ratio, c.exponent));
    double result = free;
    if (gap) {
        finite(*gap, "IDM gap");
        if (*gap <= 0) throw std::invalid_argument("nonpositive IDM gap");
        const double dynamic = v*c.headway + v*(v-leader_speed)/(2*std::sqrt(c.acceleration*c.comfortable_braking));
        const double desired = c.standstill_gap + std::max(0.0, dynamic);
        const double term = desired / *gap;
        result = c.acceleration * (1 - std::pow(ratio, c.exponent) - term*term);
    }
    finite(result, "IDM acceleration");
    return result;
}

double Motion::position(double t) const {
    const double moving = std::min(t, moving_until);
    return x + v*moving + 0.5*acceleration*moving*moving;
}
double Motion::speed(double t) const {
    return t > moving_until ? 0.0 : std::max(0.0, v + acceleration*t);
}
Motion ballistic(double x, double v, double acceleration, double dt) {
    finite(x,"position"); finite(v,"speed"); finite(acceleration,"acceleration"); finite(dt,"interval");
    if (v < 0 || dt <= 0) throw std::invalid_argument("invalid ballistic input");
    if (v == 0 && acceleration <= 0) return {x,0,0,0};
    double until = dt;
    if (acceleration < 0 && v + acceleration*dt < 0) until = -v/acceleration;
    Motion m{x,v,acceleration,until};
    finite(m.position(dt),"ballistic position");
    finite(m.speed(dt),"ballistic speed");
    if (m.position(dt) < x || m.speed(dt) < 0) throw std::runtime_error("backwards or negative ballistic trajectory");
    return m;
}

void validate_trajectory(const Motion& leader, const Motion& follower, double length, double dt) {
    finite(length,"vehicle length"); finite(dt,"interval");
    if (length <= 0 || dt <= 0) throw std::invalid_argument("invalid separation geometry");
    validate_motion(leader,dt); validate_motion(follower,dt);
    std::array<double,4> cuts{0, dt, leader.moving_until, follower.moving_until};
    std::sort(cuts.begin(), cuts.end());
    auto gap = [&](double t) { return leader.position(t) - follower.position(t) - length; };
    for (std::size_t i=1; i<cuts.size(); ++i) {
        double lo=std::clamp(cuts[i-1],0.0,dt), hi=std::clamp(cuts[i],0.0,dt);
        if (hi < lo) continue;
        const double g0=gap(lo), g1=gap(hi);
        finite(g0,"separation"); finite(g1,"separation");
        if (g0 <= 0 || g1 <= 0) throw std::runtime_error("nonpositive vehicle separation");
        const double mid=(lo+hi)/2;
        const double relative_a = (mid >= leader.moving_until && leader.moving_until < dt ? 0 : leader.acceleration) -
                                  (mid >= follower.moving_until && follower.moving_until < dt ? 0 : follower.acceleration);
        if (relative_a > 0) {
            const double relative_v = leader.speed(lo) - follower.speed(lo);
            const double minimum_time=lo-relative_v/relative_a;
            if (minimum_time > lo && minimum_time < hi) {
                const double minimum=gap(minimum_time);
                finite(minimum,"interior separation");
                if (minimum <= 0) throw std::runtime_error("interior vehicle overlap");
            }
        }
    }
}
void validate_line(const Motion& motion, double length, double line, double dt) {
    finite(length,"vehicle length"); finite(line,"stop line"); finite(dt,"interval");
    validate_motion(motion,dt);
    const double front = motion.position(dt) + length/2;
    finite(front,"front position");
    if (front > line) throw std::runtime_error("unauthorized stop-line crossing");
}
bool full_stop_interval(const Motion& motion, double length, bool eligible, double dt) {
    finite(length,"vehicle length"); finite(dt,"interval");
    if (length<=0 || dt<=0) throw std::invalid_argument("invalid stop geometry");
    validate_motion(motion,dt);
    const double start_front=motion.x+length/2;
    const double end_front=motion.position(dt)+length/2;
    return eligible && start_front>=399.25 && end_front<=399.75 &&
           motion.v<=0.1 && motion.speed(dt)<=0.1;
}

World::World() : World(Config{}, canonical()) {}
World::World(Config config, std::vector<Vehicle> vehicles) : config_(config), initial_(std::move(vehicles)) {
    validate_config(config_, initial_); reset();
}
void World::reset() {
    active_=initial_; records_.clear();
    for (const auto& car : initial_) records_.push_back({car.id});
    tick_=0; released_=false; release_pending_=false; invalid_=false; release_tick_.reset(); diagnostic_.clear();
}
void World::request_release() { if (!released_ && !invalid_ && !complete()) release_pending_=true; }
int World::completed_count() const {
    return static_cast<int>(std::count_if(records_.begin(), records_.end(), [](const Record& r){ return r.completion_tick.has_value(); }));
}
void World::step() {
    if (complete() || invalid_) return;
    if (release_pending_) { released_=true; release_pending_=false; release_tick_=tick_; }
    std::vector<Motion> proposed;
    std::vector<bool> permitted;
    proposed.reserve(active_.size()); permitted.reserve(active_.size());
    int affected=0;
    try {
        int eligible=-1;
        for (std::size_t i=0;i<active_.size();++i) if (!active_[i].crossing_tick) {eligible=static_cast<int>(i);break;}
        for (std::size_t i=0;i<active_.size();++i) {
            const auto& car=active_[i]; affected=car.id;
            const bool permission=released_ && car.qualified;
            permitted.push_back(permission);
            std::optional<double> real_gap;
            if (i) real_gap=active_[i-1].x-car.x-config_.length;
            double alpha;
            if (real_gap) alpha=idm(config_,car.v,real_gap,active_[i-1].v);
            else alpha=idm(config_,car.v);
            if (!permission) {
                const double virtual_gap=config_.virtual_obstacle-(car.x+config_.length/2);
                const double stop_alpha=idm(config_,car.v,virtual_gap,0);
                alpha=real_gap ? std::min(alpha,stop_alpha) : stop_alpha;
            }
            proposed.push_back(ballistic(car.x,car.v,alpha));
        }
        for (std::size_t i=0;i<active_.size();++i) {
            affected=active_[i].id;
            const auto& m=proposed[i];
            if (m.position(step_seconds) < active_[i].x || m.speed(step_seconds) < 0)
                throw std::runtime_error("nonmonotone motion");
            if (i) validate_trajectory(proposed[i-1],m,config_.length);
            if (!permitted[i]) validate_line(m,config_.length,config_.stop_line);
        }
        auto next=active_;
        auto records=records_;
        for (std::size_t i=0;i<next.size();++i) {
            auto& car=next[i]; const auto& m=proposed[i];
            const double new_front=m.position(step_seconds)+config_.length/2;
            const bool qualifies=full_stop_interval(m,config_.length,
                static_cast<int>(i)==eligible && !car.crossing_tick);
            if (!car.qualified) {
                car.dwell=qualifies ? car.dwell+1 : 0;
                if (car.dwell>=20) {car.qualified=true; car.qualification_tick=tick_+1;}
            }
            car.x=m.position(step_seconds); car.v=m.speed(step_seconds);
            auto& record=*std::find_if(records.begin(),records.end(),[&](const Record& r){return r.id==car.id;});
            if (car.qualification_tick) record.qualification_tick=car.qualification_tick;
            if (new_front>config_.stop_line && !car.crossing_tick) {
                if (!permitted[i]) throw std::runtime_error("crossing without start-of-interval permission");
                car.crossing_tick=tick_+1; record.crossing_tick=car.crossing_tick;
            }
            if (car.x-config_.length/2>=config_.lane_end) {
                car.completion_tick=tick_+1; record.completion_tick=car.completion_tick;
                record.completion_x=car.x; record.completion_v=car.v;
            }
        }
        next.erase(std::remove_if(next.begin(),next.end(),[](const Vehicle& v){return v.completion_tick.has_value();}),next.end());
        active_=std::move(next); records_=std::move(records); ++tick_;
    } catch (const std::exception& error) {
        invalid_=true;
        std::ostringstream message;
        message << "M2 canonical fixture invalid at attempted interval " << tick_ << "->" << tick_+1
                << ", vehicle " << affected << ": " << error.what();
        if (release_tick_) message << ", release applied at tick " << *release_tick_;
        for (const auto& car:active_) message << " [ID " << car.id << " x=" << car.x << " v=" << car.v << "]";
        diagnostic_=message.str();
    }
}

void Driver::reset() {world_.reset(); previous_.clear(); running_=false; playback_=1; backlog_ns_=0;scheduled_release_tick_.reset();}
void Driver::schedule_release(std::uint64_t tick) {
    if (tick < world_.tick()) throw std::invalid_argument("release tick is in the past");
    scheduled_release_tick_=tick;
}
void Driver::set_playback(int multiplier) {
    if (multiplier!=1 && multiplier!=2 && multiplier!=4) throw std::invalid_argument("playback must be 1, 2, or 4");
    playback_=multiplier;
}
void Driver::advance(std::int64_t elapsed_ns, std::uint32_t max_steps) {
    if (elapsed_ns<0) throw std::invalid_argument("elapsed time must be nonnegative");
    if (!running_ || world_.complete() || world_.invalid()) return;
    if (elapsed_ns>(std::numeric_limits<std::int64_t>::max()-backlog_ns_)/playback_)
        throw std::overflow_error("elapsed time exceeds driver capacity");
    backlog_ns_+=elapsed_ns*playback_;
    for (std::uint32_t i=0;i<max_steps && backlog_ns_>=step_ns && !world_.complete() && !world_.invalid();++i) {
        if (scheduled_release_tick_ && world_.tick()==*scheduled_release_tick_) {
            world_.request_release(); scheduled_release_tick_.reset();
        }
        previous_=world_.active(); world_.step();
        if (!world_.invalid()) backlog_ns_-=step_ns;
    }
    if (world_.complete() || world_.invalid()) {running_=false;backlog_ns_=0;}
}
double Driver::display_x(int id) const {
    const auto& cars=world_.active();
    const auto it=std::find_if(cars.begin(),cars.end(),[&](const Vehicle& v){return v.id==id;});
    if (it==cars.end()) throw std::invalid_argument("inactive vehicle ID");
    if (backlog_ns_>=step_ns || previous_.empty()) return it->x;
    const auto prior=std::find_if(previous_.begin(),previous_.end(),[&](const Vehicle& v){return v.id==id;});
    if (prior==previous_.end()) return it->x;
    const double fraction=std::clamp(static_cast<double>(backlog_ns_)/step_ns,0.0,1.0);
    return prior->x+(it->x-prior->x)*fraction;
}
} // namespace traffic::m2
