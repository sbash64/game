cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0048 NEW)
project(sbash64-sdl-learn LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG release-2.0.16)
FetchContent_MakeAvailable(SDL2)

add_executable(sbash64-sdl-learn-main main.cpp)
target_link_libraries(sbash64-sdl-learn-main SDL2::SDL2)
