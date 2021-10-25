cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0048 NEW)
project(sbash64-game LANGUAGES CXX)

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

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(sbash64-game-main sdl-wrappers.cpp alsa-wrappers.cpp main.cpp)
target_link_libraries(sbash64-game-main SDL2::image SDL2::SDL2 asound
                      Threads::Threads sndfile)
target_include_directories(sbash64-game-main PRIVATE include)
target_compile_features(sbash64-game-main PRIVATE cxx_std_20)
target_compile_options(sbash64-game-main PRIVATE "${SBASH64_GAME_WARNINGS}")
