cmake_minimum_required(VERSION 3.25)
project(TrafficSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(TRAFFIC_BUILD_APP "Build the SDL3 application" ON)
include(CTest)
include(cmake/Dependencies.cmake)

add_library(traffic_core src/core/traffic.cpp)
target_include_directories(traffic_core PUBLIC src/core)
target_compile_features(traffic_core PUBLIC cxx_std_20)

add_executable(traffic_headless src/headless/main.cpp)
target_link_libraries(traffic_headless PRIVATE traffic_core)

if(TRAFFIC_BUILD_APP)
    traffic_fetch_app_dependencies()
    add_library(traffic_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
    target_include_directories(traffic_imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(traffic_imgui PUBLIC SDL3::SDL3)
    add_executable(traffic_app src/app/main.cpp)
    target_link_libraries(traffic_app PRIVATE traffic_core traffic_imgui SDL3::SDL3)
endif()

if(BUILD_TESTING)
    traffic_fetch_test_dependencies()
    add_executable(traffic_tests tests/traffic_tests.cpp tests/pacing_tests.cpp)
    target_include_directories(traffic_tests PRIVATE src/app)
    target_link_libraries(traffic_tests PRIVATE traffic_core Catch2::Catch2WithMain)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(traffic_tests)
    add_test(NAME headless_baseline COMMAND traffic_headless)
endif()
