cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0048 NEW)
project(sbash64-sdl-learn LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG release-2.0.16)
FetchContent_MakeAvailable(SDL2)

FetchContent_Declare(
  SDL_image
  GIT_REPOSITORY https://github.com/madebr/SDL_image
  GIT_TAG 3e63e767bd33f0ae00eee31c407e0a608422be47)
FetchContent_MakeAvailable(SDL_image)

add_executable(sbash64-sdl-learn-main main.cpp)
target_link_libraries(sbash64-sdl-learn-main SDL2::image SDL2::SDL2)
target_compile_features(sbash64-sdl-learn-main PRIVATE cxx_std_20)
