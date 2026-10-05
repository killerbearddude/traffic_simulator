#include "traffic.hpp"
#include "frame_pacing.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

void draw_road(SDL_Renderer* renderer, int width, int height, double fraction, bool vehicle_active) {
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);
    const float top = std::min(335.0f, h * 0.57f);
    const float road_y = top + (h - top) * 0.5f;
    const float road_h = std::max(32.0f, std::min(90.0f, (h - top) * 0.48f));
    SDL_FRect road{0.0f, road_y - road_h / 2.0f, w, road_h};
    SDL_SetRenderDrawColor(renderer, 43, 48, 55, 255);
    SDL_RenderFillRect(renderer, &road);
    SDL_SetRenderDrawColor(renderer, 230, 198, 92, 255);
    for (float x = 10.0f; x < w; x += 42.0f) {
        SDL_FRect dash{x, road_y - 1.5f, std::min(23.0f, w - x), 3.0f};
        SDL_RenderFillRect(renderer, &dash);
    }
    if (vehicle_active) {
        const float left = 30.0f;
        const float right = std::max(left, w - 30.0f);
        const float center_x = left + static_cast<float>(fraction) * (right - left);
        SDL_FRect car{center_x - 18.0f, road_y - 12.0f, 36.0f, 24.0f};
        SDL_SetRenderDrawColor(renderer, 70, 170, 245, 255);
        SDL_RenderFillRect(renderer, &car);
    }
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
    SDL_Window* window = SDL_CreateWindow("Traffic Simulator TS-001", 960, 640, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
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

    traffic::Driver driver;
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
        ImGui::SetNextWindowSize(ImVec2(std::max(120.0f, std::min(355.0f, static_cast<float>(width) - 20.0f)),
                                         std::max(120.0f, std::min(315.0f, static_cast<float>(height) * 0.52f))), ImGuiCond_Always);
        ImGui::Begin("Single lane", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        if (ImGui::Button(driver.running() ? "Pause" : "Run", ImVec2(76, 0)) && !driver.world().complete()) {
            driver.set_running(!driver.running());
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(76, 0))) driver.reset();
        ImGui::TextUnformatted("Playback");
        for (int speed : {1, 2, 4}) {
            if (speed != 1) ImGui::SameLine();
            char label[16];
            std::snprintf(label, sizeof(label), "%dx", speed);
            if (ImGui::RadioButton(label, driver.playback() == speed)) driver.set_playback(speed);
        }
        const auto& world = driver.world();
        ImGui::Separator();
        ImGui::Text("State: %s", world.complete() ? "Complete" : driver.running() ? "Running" : "Paused");
        ImGui::Text("Time: %.2f s   Tick: %llu", world.time(), static_cast<unsigned long long>(world.tick()));
        ImGui::Text("Distance: %.2f m / %.2f m", world.distance(), world.fixture().lane_length);
        ImGui::Text("Speed: %.2f m/s", world.fixture().speed);
        ImGui::Text("Active: %d   Completed: %d", world.active_count(), world.completed_count());
        ImGui::Text("Playback: %dx", driver.playback());
        if (world.complete()) {
            const auto record = world.completion();
            ImGui::Text("Finished: tick %llu, %.2f s, %.2f m",
                        static_cast<unsigned long long>(record.tick), record.time, record.distance);
        }
        ImGui::End();

        SDL_SetRenderDrawColor(renderer, 23, 29, 35, 255);
        SDL_RenderClear(renderer);
        draw_road(renderer, width, height, driver.display_distance() / world.fixture().lane_length, !world.complete());
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
