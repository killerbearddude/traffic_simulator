#include "m2.hpp"
#include "frame_pacing.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

void draw_view(const char* title, float top, float height, int window_width,
               double first, double last, const traffic::m2::Driver& driver) {
    ImGui::SetNextWindowPos(ImVec2(10,top),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(std::max(180,window_width-20),std::max(95.0f,height)),ImGuiCond_Always);
    ImGui::Begin(title,nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
    const ImVec2 origin=ImGui::GetCursorScreenPos();
    const ImVec2 available=ImGui::GetContentRegionAvail();
    auto* draw=ImGui::GetWindowDrawList();
    const float left=origin.x+18, right=origin.x+std::max(30.0f,available.x-18);
    const float scale=(right-left)/static_cast<float>(last-first);
    const float cy=origin.y+available.y*.56f;
    const auto px=[&](double x){return left+static_cast<float>(x-first)*scale;};
    draw->PushClipRect(origin,ImVec2(origin.x+available.x,origin.y+available.y),true);
    draw->AddRectFilled(ImVec2(left,cy-2*scale),ImVec2(right,cy+2*scale),IM_COL32(45,51,59,255));
    for (double marker=std::ceil(first/50)*50;marker<=last;marker+=50)
        draw->AddLine(ImVec2(px(marker),cy+3*scale),ImVec2(px(marker),cy+3*scale+5),IM_COL32(140,150,160,255));
    const float line=px(driver.world().config().stop_line);
    if (line>=left && line<=right) {
        draw->AddLine(ImVec2(line,origin.y+8),ImVec2(line,origin.y+available.y-6),IM_COL32(245,188,70,255),2);
        draw->AddText(ImVec2(std::min(line+4,right-70),origin.y+5),IM_COL32(245,188,70,255),"400 m stop line");
    }
    const auto& config=driver.world().config();
    for (const auto& car:driver.world().active()) {
        const float x=px(driver.display_x(car.id));
        if (x<left-config.length*scale || x>right+config.length*scale) continue;
        draw->AddRectFilled(ImVec2(x-static_cast<float>(config.length/2)*scale,cy-static_cast<float>(config.width/2)*scale),
            ImVec2(x+static_cast<float>(config.length/2)*scale,cy+static_cast<float>(config.width/2)*scale),
            car.qualified ? IM_COL32(90,215,130,255) : IM_COL32(70,170,245,255));
        draw->AddText(ImVec2(x-4,cy+std::max(5.0f,static_cast<float>(config.width/2)*scale+2)),
                      IM_COL32(240,240,240,255),std::to_string(car.id).c_str());
    }
    draw->PopClipRect();
    ImGui::Dummy(available);
    ImGui::End();
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 2 || (argc == 2 && std::strcmp(argv[1], "--force-fallback") != 0)) {
        std::fprintf(stderr, "Usage: traffic_app [--force-fallback]\n");
        return 1;
    }
    const bool force_fallback = argc == 2;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Traffic Simulator M2", 1100, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    // The fixed controls and two views need this much space to remain inside the window.
    if (!SDL_SetWindowMinimumSize(window, 960, 640)) {
        std::fprintf(stderr, "SDL_SetWindowMinimumSize failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    bool vsync_succeeded = false;
    if (force_fallback) {
        int actual_vsync = -1;
        if (!SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED) ||
            !SDL_GetRenderVSync(renderer, &actual_vsync) || actual_vsync != SDL_RENDERER_VSYNC_DISABLED) {
            std::fprintf(stderr, "Could not verify forced VSync disable: %s\n", SDL_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
        std::fprintf(stderr, "Pacing: timed fallback at nominal 60 FPS (forced; VSync disabled)\n");
    } else {
        vsync_succeeded = SDL_SetRenderVSync(renderer, 1);
        if (vsync_succeeded) {
            std::fprintf(stderr, "Pacing: VSync requested successfully\n");
        } else {
            std::fprintf(stderr, "VSync unavailable: %s\n", SDL_GetError());
            if (!SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED)) {
                std::fprintf(stderr, "Could not explicitly disable VSync for fallback: %s\n", SDL_GetError());
            }
            std::fprintf(stderr, "Pacing: timed fallback at nominal 60 FPS\n");
        }
    }
    const auto pacing_mode = traffic::app::select_pacing(force_fallback, vsync_succeeded);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer)) {
        std::fprintf(stderr, "ImGui SDL3 initialization failed\n");
        ImGui::DestroyContext();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
        std::fprintf(stderr, "ImGui SDL renderer initialization failed\n");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    traffic::m2::Driver driver;
    bool open = true;
    std::uint64_t last_ns = SDL_GetTicksNS();
    while (open) {
        const std::uint64_t frame_start_ns = SDL_GetTicksNS();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))) {
                open = false;
            }
        }
        if (!open) break;
        const std::uint64_t now_ns = SDL_GetTicksNS();
        driver.advance(static_cast<std::int64_t>(now_ns - last_ns));
        last_ns = now_ns;

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
        const float controls_h=std::min(265.0f,std::max(165.0f,static_cast<float>(height)*.38f));
        ImGui::SetNextWindowSize(ImVec2(std::max(180,width-20),controls_h), ImGuiCond_Always);
        ImGui::Begin("M2 queue controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        if (ImGui::Button(driver.running() ? "Pause" : "Run", ImVec2(76, 0)) && !driver.world().complete() && !driver.world().invalid()) {
            driver.set_running(!driver.running());
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(76, 0))) driver.reset();
        ImGui::SameLine();
        if (ImGui::Button("Release queue") && !driver.world().released()) driver.request_release();
        ImGui::TextUnformatted("Playback");
        for (int speed : {1, 2, 4}) {
            if (speed != 1) ImGui::SameLine();
            char label[16];
            std::snprintf(label, sizeof(label), "%dx", speed);
            if (ImGui::RadioButton(label, driver.playback() == speed)) driver.set_playback(speed);
        }
        const auto& world = driver.world();
        ImGui::Separator();
        ImGui::Text("State: %s", world.invalid() ? "Invalid" : world.complete() ? "Complete" : driver.running() ? "Running" : "Paused");
        ImGui::Text("Time: %.2f s   Tick: %llu", world.time(), static_cast<unsigned long long>(world.tick()));
        ImGui::Text("Queue: %s   Active: %zu   Completed: %d", world.released() ? "Released" :
                    world.release_pending() ? "Release pending" : "Held",
                    world.active().size(), world.completed_count());
        ImGui::Text("Playback: %dx", driver.playback());
        if (world.invalid()) ImGui::TextWrapped("Diagnostic: %s",world.diagnostic().c_str());
        for (const auto& car:world.active())
            ImGui::Text("ID %d  x %.2f m  v %.2f m/s  dwell %d/20  %s",car.id,car.x,car.v,car.dwell,
                        car.qualified ? "qualified" : "waiting");
        for (const auto& record:world.records()) {
            if (!record.qualification_tick && !record.crossing_tick && !record.completion_tick) continue;
            ImGui::Text("ID %d events: stop %s  line %s  clear %s",record.id,
                record.qualification_tick ? std::to_string(*record.qualification_tick).c_str() : "-",
                record.crossing_tick ? std::to_string(*record.crossing_tick).c_str() : "-",
                record.completion_tick ? std::to_string(*record.completion_tick).c_str() : "-");
        }
        ImGui::End();

        const float remaining=std::max(200.0f,static_cast<float>(height)-controls_h-35.0f);
        draw_view("Overview 0-600 m",controls_h+15,remaining*.5f,width,0,610,driver);
        draw_view("Stop-line detail 340-440 m",controls_h+20+remaining*.5f,remaining*.5f-5,width,340,440,driver);

        SDL_SetRenderDrawColor(renderer, 23, 29, 35, 255);
        SDL_RenderClear(renderer);
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
        traffic::app::finish_frame(pacing_mode, frame_start_ns, SDL_GetTicksNS(), [](std::uint64_t duration_ns) {
            SDL_DelayNS(duration_ns);
        });
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
