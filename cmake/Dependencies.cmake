include(FetchContent)

# Immutable upstream release commits. FetchContent uses these exact revisions.
function(traffic_fetch_app_dependencies)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(sdl3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG a96677bdf6b4acb84af4ec294e5f60a4e8cbbe03)
    FetchContent_MakeAvailable(sdl3)
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG f5befd2d29e66809cd1110a152e375a7f1981f06)
    FetchContent_MakeAvailable(imgui)
    set(imgui_SOURCE_DIR "${imgui_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

function(traffic_fetch_test_dependencies)
    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
    set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG 2b60af89e23d28eefc081bc930831ee9d45ea58b)
    FetchContent_MakeAvailable(catch2)
    set(catch2_SOURCE_DIR "${catch2_SOURCE_DIR}" PARENT_SCOPE)
endfunction()
