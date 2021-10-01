#include <SDL.h>
#include <SDL_events.h>
#include <SDL_stdinc.h>

#include <iostream>
#include <map>

enum class Color { red, blue, green, white };

int main() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
              << '\n';
    return 1;
  }
  auto *window =
      SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError()
              << '\n';
    return 1;
  }
  auto *screenSurface = SDL_GetWindowSurface(window);
  std::map<Color, Uint32> colorCodes;
  colorCodes[Color::red] = SDL_MapRGB(screenSurface->format, 0xFF, 0, 0);
  colorCodes[Color::green] = SDL_MapRGB(screenSurface->format, 0, 0xFF, 0);
  colorCodes[Color::blue] = SDL_MapRGB(screenSurface->format, 0, 0, 0xFF);
  colorCodes[Color::white] =
      SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF);
  auto colorCode{colorCodes.at(Color::white)};
  auto running{true};
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
      running = event.type != SDL_QUIT;
    if (event.type == SDL_KEYDOWN)
      switch (event.key.keysym.sym) {
      case SDLK_UP:
        break;
      case SDLK_DOWN:
        colorCode = colorCodes.at(Color::green);
        break;
      case SDLK_LEFT:
        colorCode = colorCodes.at(Color::blue);
        break;
      case SDLK_RIGHT:
        colorCode = colorCodes.at(Color::red);
        break;
      default:
        break;
      }
    SDL_FillRect(screenSurface, nullptr, colorCode);
    SDL_UpdateWindowSurface(window);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
}