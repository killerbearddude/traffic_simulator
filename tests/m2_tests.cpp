#include "m2.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace traffic::m2;
using Catch::Approx;

TEST_CASE("m2_IDM equation and invalid inputs") {
    Config c;
    REQUIRE(idm(c,0)==1.5);
    REQUIRE(idm(c,c.desired_speed)==0.0);
    REQUIRE(idm(c,0,2.0,0)==0.0);
    const double v=10, leader=12, gap=30;
    const double desired=2+std::max(0.0,v*1.5+v*(v-leader)/(2*std::sqrt(3.0)));
    REQUIRE(idm(c,v,gap,leader)==Approx(1.5*(1-std::pow(v/c.desired_speed,4)-std::pow(desired/gap,2))));
    REQUIRE(idm(c,15,15,0)<idm(c,15));
    REQUIRE(idm(c,1,10,50)==Approx(1.5*(1-std::pow(1/c.desired_speed,4)-std::pow(2.0/10,2))));
    REQUIRE_THROWS(idm(c,1,0.0));
    REQUIRE_THROWS(idm(c,std::numeric_limits<double>::infinity()));
    c.comfortable_braking=0;
    REQUIRE_THROWS(idm(c,1,5.0));
}
TEST_CASE("m2_ballistic motion and stops") {
    const auto accelerating=ballistic(0,2,2);
    REQUIRE(accelerating.position(.05)==Approx(.1025)); REQUIRE(accelerating.speed(.05)==Approx(2.1));
    const auto constant=ballistic(0,2,0);
    REQUIRE(constant.position(.05)==Approx(.1)); REQUIRE(constant.speed(.05)==Approx(2));
    const auto braking=ballistic(0,2,-2);
    REQUIRE(braking.position(.05)==Approx(.0975)); REQUIRE(braking.speed(.05)==Approx(1.9));
    const auto early=ballistic(0,.05,-2);
    REQUIRE(early.moving_until==Approx(.025)); REQUIRE(early.position(.05)==Approx(.000625)); REQUIRE(early.speed(.05)==0);
    const auto exact=ballistic(0,.1,-2);
    REQUIRE(exact.position(.05)==Approx(.0025)); REQUIRE(exact.speed(.05)==0);
    const auto still=ballistic(7,0,-2);
    REQUIRE(still.position(.05)==7); REQUIRE(still.speed(.05)==0);
}
TEST_CASE("m2_swept safety detects interior overlap and stop transitions") {
    // At both ends the gap is positive; the follower catches the leader in the middle.
    Motion leader{10,0,40,.05}, follower{9,2,-40,.05};
    REQUIRE(leader.position(0)-follower.position(0)-.99>0);
    REQUIRE(leader.position(.05)-follower.position(.05)-.99>0);
    REQUIRE_THROWS(validate_trajectory(leader,follower,.99));
    const auto stopping_leader=ballistic(10,.2,-10);
    const auto follower2=ballistic(9,.5,-20);
    REQUIRE_NOTHROW(validate_trajectory(stopping_leader,follower2,.5));
    REQUIRE_THROWS(validate_line(ballistic(399.45,2,0),1,400));
    REQUIRE_NOTHROW(validate_line(ballistic(399,0,0),1,400));
    REQUIRE_THROWS(ballistic(0,-1,0));
    REQUIRE_THROWS(ballistic(0,1,std::numeric_limits<double>::quiet_NaN()));
    REQUIRE_THROWS(validate_trajectory(Motion{10,-1,0,.05},Motion{0,0,0,0},1));
    REQUIRE_THROWS(validate_line(Motion{399,1,std::numeric_limits<double>::quiet_NaN(),.05},1,400));
}
TEST_CASE("m2_fixture checkpoint and event order") {
    World world;
    const auto& c=world.config();
    REQUIRE(c.lane_end==600); REQUIRE(c.stop_line==400); REQUIRE(c.length==4.5); REQUIRE(c.width==1.8);
    REQUIRE(c.desired_speed==65.0/3.6); REQUIRE(world.active().size()==6);
    for (int i=0;i<6;++i) {REQUIRE(world.active()[i].id==i+1); REQUIRE(world.active()[i].x==240-40*i);}
    for (int i=0;i<1200;++i) {world.step(); REQUIRE_FALSE(world.invalid());}
    REQUIRE(world.tick()==1200); REQUIRE(world.active().size()==6); REQUIRE(world.completed_count()==0);
    for (const auto& car:world.active()) {
        REQUIRE(car.x+c.length/2<=400); REQUIRE(car.v<=.1); REQUIRE(car.qualified==(car.id==1));
        REQUIRE_FALSE(car.crossing_tick);
    }
    world.request_release(); world.step();
    REQUIRE(world.release_tick()==1200);
    while (!world.complete() && !world.invalid() && world.tick()<3600) world.step();
    REQUIRE_FALSE(world.invalid()); REQUIRE(world.complete()); REQUIRE(world.tick()<=3600);
    REQUIRE(world.completed_count()==6); REQUIRE(world.records().size()==6);
    for (const auto& r:world.records()) {
        REQUIRE(r.qualification_tick); REQUIRE(r.crossing_tick); REQUIRE(r.completion_tick);
        REQUIRE(*r.qualification_tick<*r.crossing_tick); REQUIRE(*r.crossing_tick<*r.completion_tick);
        REQUIRE(r.completion_x>=602.25);
    }
    const auto frozen=world.tick(); world.step(); REQUIRE(world.tick()==frozen);
}
TEST_CASE("m2_held timeout leaves vehicles and records intact") {
    World world;
    for (int i=0;i<3600;++i) world.step();
    REQUIRE_FALSE(world.invalid()); REQUIRE_FALSE(world.complete());
    REQUIRE(world.active().size()==6); REQUIRE(world.completed_count()==0);
    for (const auto& r:world.records()) {REQUIRE_FALSE(r.crossing_tick); REQUIRE_FALSE(r.completion_tick);}
}
TEST_CASE("m2_early release still requires individual stops and reset clears run") {
    World world; world.request_release(); world.request_release();
    while (!world.complete() && !world.invalid() && world.tick()<3600) world.step();
    REQUIRE(world.release_tick()==0); REQUIRE(world.complete()); REQUIRE_FALSE(world.invalid());
    for (const auto& r:world.records()) {
        REQUIRE(r.qualification_tick); REQUIRE(r.crossing_tick);
        REQUIRE(*r.qualification_tick<*r.crossing_tick);
    }
    world.reset(); REQUIRE(world.tick()==0); REQUIRE_FALSE(world.released());
    REQUIRE_FALSE(world.release_tick()); REQUIRE(world.completed_count()==0);
    for (const auto& r:world.records()) REQUIRE_FALSE(r.qualification_tick);
}
TEST_CASE("m2_driver release in batch pause backlog and reset") {
    Driver driver; driver.set_running(true);
    driver.advance(60'000'000'000LL,1200);
    REQUIRE(driver.world().tick()==1200); REQUIRE(driver.backlog_ns()==0);
    driver.set_running(false); driver.advance(1'000'000'000);
    REQUIRE(driver.world().tick()==1200);
    driver.request_release(); driver.set_running(true);
    driver.advance(500'000'000,2);
    REQUIRE(driver.world().release_tick()==1200); REQUIRE(driver.world().tick()==1202);
    REQUIRE(driver.backlog_ns()==400'000'000);
    driver.reset(); REQUIRE(driver.world().tick()==0); REQUIRE_FALSE(driver.world().released());
    REQUIRE(driver.backlog_ns()==0); REQUIRE_FALSE(driver.running()); REQUIRE(driver.playback()==1);
}
TEST_CASE("m2_configuration rejects overlap and duplicate IDs") {
    Config c;
    REQUIRE_THROWS(World(c,{{1,240,0},{1,200,0}}));
    REQUIRE_THROWS(World(c,{{1,240,0},{2,239,0}}));
    c.desired_speed=std::numeric_limits<double>::quiet_NaN();
    REQUIRE_THROWS(World(c,{{1,240,0}}));
}
TEST_CASE("m2_lower acceleration wins between real leader and stop constraint") {
    World world;
    const auto& c=world.config();
    const auto first=world.active()[0], second=world.active()[1];
    const double real=idm(c,second.v,first.x-second.x-c.length,first.v);
    const double virtual_a=idm(c,second.v,c.virtual_obstacle-(second.x+c.length/2),0);
    REQUIRE(real<virtual_a);
    world.step();
    REQUIRE(world.active()[1].v==Approx(second.v+std::min(real,virtual_a)*.05));
}

TEST_CASE("m2_dwell requires 20 full intervals and next interval permission") {
    Config c;
    World world(c,{{1,397.25,0},{2,390,0}});
    world.request_release();
    for (int i=0;i<19;++i) world.step();
    REQUIRE_FALSE(world.invalid()); REQUIRE(world.active()[0].dwell==19);
    REQUIRE_FALSE(world.active()[0].qualified);
    REQUIRE(world.active()[1].dwell==0);
    world.step();
    REQUIRE(world.active()[0].dwell==20);
    REQUIRE(world.active()[0].qualified);
    REQUIRE(world.active()[0].qualification_tick==20);
    REQUIRE(world.active()[0].x==Approx(397.25));
    world.step();
    REQUIRE(world.active()[0].x>397.25);
    REQUIRE(world.release_tick()==0);
}
TEST_CASE("m2_swept invariant and six vehicle accounting every tick") {
    World world;
    std::vector<Vehicle> previous=world.active();
    for (int k=0;k<3600 && !world.complete() && !world.invalid();++k) {
        if (world.tick()==1200) world.request_release();
        const auto old_tick=world.tick();
        world.step();
        REQUIRE(world.tick()==old_tick+1);
        REQUIRE(static_cast<int>(world.active().size())+world.completed_count()==6);
        for (std::size_t i=0;i<world.active().size();++i) {
            const auto& car=world.active()[i];
            REQUIRE(std::isfinite(car.x)); REQUIRE(std::isfinite(car.v)); REQUIRE(car.v>=0);
            const auto prior=std::find_if(previous.begin(),previous.end(),[&](const Vehicle& v){return v.id==car.id;});
            REQUIRE(prior!=previous.end()); REQUIRE(car.x>=prior->x);
            if (i) REQUIRE(world.active()[i-1].x-car.x>world.config().length);
            if (!car.qualified || !world.released()) REQUIRE(car.x+world.config().length/2<=400);
        }
        previous=world.active();
    }
    REQUIRE_FALSE(world.invalid()); REQUIRE(world.complete());
}
TEST_CASE("m2_playback schedules reproduce every committed tick") {
    World reference;
    std::vector<std::vector<Vehicle>> expected;
    while (!reference.complete() && !reference.invalid() && reference.tick()<3600) {
        if (reference.tick()==1200) reference.request_release();
        reference.step(); expected.push_back(reference.active());
    }
    REQUIRE(reference.complete());
    for (int playback:{1,2,4}) for (int fps:{30,60,144,73}) {
        Driver driver; driver.set_playback(playback); driver.set_running(true);
        std::size_t compared=0; int frames=0;
        const std::int64_t base=1'000'000'000/fps;
        while (!driver.world().complete() && !driver.world().invalid() && frames<20000) {
            const auto elapsed=fps==73 ? base+(frames%3-1)*1'000'000 : base;
            driver.advance(elapsed,0);
            while (driver.backlog_ns()>=Driver::step_ns && !driver.world().complete() && !driver.world().invalid()) {
                if (driver.world().tick()==1200) driver.request_release();
                driver.advance(0,1);
                REQUIRE(driver.world().tick()==compared+1);
                const auto& actual=driver.world().active();
                REQUIRE(actual.size()==expected[compared].size());
                for (std::size_t i=0;i<actual.size();++i) {
                    REQUIRE(actual[i].id==expected[compared][i].id);
                    REQUIRE(actual[i].x==expected[compared][i].x);
                    REQUIRE(actual[i].v==expected[compared][i].v);
                    REQUIRE(actual[i].dwell==expected[compared][i].dwell);
                    REQUIRE(actual[i].qualified==expected[compared][i].qualified);
                }
                ++compared;
            }
            ++frames;
        }
        REQUIRE_FALSE(driver.world().invalid()); REQUIRE(driver.world().complete());
        REQUIRE(compared==expected.size());
        REQUIRE(driver.world().records().size()==reference.records().size());
        for (std::size_t i=0;i<reference.records().size();++i) {
            REQUIRE(driver.world().records()[i].qualification_tick==reference.records()[i].qualification_tick);
            REQUIRE(driver.world().records()[i].crossing_tick==reference.records()[i].crossing_tick);
            REQUIRE(driver.world().records()[i].completion_tick==reference.records()[i].completion_tick);
        }
    }
}
TEST_CASE("m2_invalid calculation latches without partial commit") {
    World world(Config{},{{1,240,1e308}});
    const auto before=world.active();
    world.request_release(); world.step();
    REQUIRE(world.invalid()); REQUIRE(world.tick()==0);
    REQUIRE(world.active().size()==1); REQUIRE(world.active()[0].x==before[0].x);
    REQUIRE(world.active()[0].v==before[0].v);
    REQUIRE(world.records()[0].qualification_tick==std::nullopt);
    REQUIRE(world.records()[0].crossing_tick==std::nullopt);
    REQUIRE(world.records()[0].completion_tick==std::nullopt);
    REQUIRE(world.diagnostic().find("nonfinite IDM acceleration")!=std::string::npos);
    world.step(); REQUIRE(world.tick()==0);
    world.reset(); REQUIRE_FALSE(world.invalid()); REQUIRE(world.tick()==0);
}
TEST_CASE("m2_scheduled release executes inside one elapsed batch") {
    Driver driver;
    driver.schedule_release(1200);
    driver.set_running(true);
    driver.advance(60'500'000'000LL,2000);
    REQUIRE_FALSE(driver.world().invalid());
    REQUIRE(driver.world().tick()==1210);
    REQUIRE(driver.world().release_tick()==1200);
    REQUIRE(driver.world().records()[0].qualification_tick==358);
    driver.reset();
    REQUIRE_FALSE(driver.world().release_tick());
    driver.set_running(true); driver.advance(50'000'000);
    REQUIRE_THROWS(driver.schedule_release(0));
}
TEST_CASE("m2_stop boundaries include edges and exclude partial intervals") {
    REQUIRE(full_stop_interval(ballistic(397.0,0,0),4.5,true)); // front 399.25
    REQUIRE(full_stop_interval(ballistic(397.5,0,0),4.5,true)); // front 399.75
    REQUIRE_FALSE(full_stop_interval(ballistic(std::nextafter(397.0,0.0),0,0),4.5,true));
    REQUIRE_FALSE(full_stop_interval(ballistic(std::nextafter(397.5,400.0),0,0),4.5,true));
    REQUIRE(full_stop_interval(ballistic(397.25,.1,-2),4.5,true));
    REQUIRE_FALSE(full_stop_interval(ballistic(397.25,std::nextafter(.1,1.0),-2),4.5,true));
    REQUIRE_FALSE(full_stop_interval(ballistic(397.25,.09,.5),4.5,true));
    REQUIRE_FALSE(full_stop_interval(ballistic(396.99,.8,-20),4.5,true)); // enters region while stopping
    REQUIRE_FALSE(full_stop_interval(ballistic(397.25,0,0),4.5,false)); // queued follower
}
TEST_CASE("m2_rear clearance keeps a downstream leader active") {
    World world;
    while (world.tick()<1200) world.step();
    world.request_release();
    bool saw_center_past_boundary=false;
    while (!world.invalid() && !world.records()[0].completion_tick && world.tick()<3600) {
        world.step();
        if (!world.active().empty() && world.active()[0].id==1 && world.active()[0].x>=600) {
            saw_center_past_boundary=true;
            REQUIRE(world.active()[0].x-world.config().length/2<600);
            REQUIRE_FALSE(world.records()[0].completion_tick);
            REQUIRE(world.active()[1].id==2);
        }
    }
    REQUIRE(saw_center_past_boundary);
    REQUIRE_FALSE(world.invalid()); REQUIRE(world.records()[0].completion_tick);
    REQUIRE(world.records()[0].completion_x-world.config().length/2>=600);
    REQUIRE(world.active()[0].id==2);
}
TEST_CASE("m2_interrupted dwell resets before qualification") {
    World world(Config{},{{1,397.0,0}});
    bool saw_credit=false, saw_reset=false;
    for (int i=0;i<30 && !world.invalid();++i) {
        const int before=world.active()[0].dwell;
        world.step();
        const int after=world.active()[0].dwell;
        saw_credit |= after>0;
        saw_reset |= before>0 && after==0;
        REQUIRE_FALSE(world.active()[0].qualified);
    }
    REQUIRE_FALSE(world.invalid()); REQUIRE(saw_credit); REQUIRE(saw_reset);
}
