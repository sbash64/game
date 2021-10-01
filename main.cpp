#include <SDL.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>

#include <iostream>
#include <map>

constexpr auto screenWidth{640};
constexpr auto screenHeight{480};

enum class Color { red, blue, green, white };

int main() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
              << '\n';
    return 1;
  }
  auto *window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED, screenWidth,
                                  screenHeight, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError()
              << '\n';
    return 1;
  }

  auto *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    std::cerr << "Renderer could not be created! SDL Error: " << SDL_GetError()
              << '\n';
    return 1;
  }

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
        break;
      case SDLK_LEFT:
        break;
      case SDLK_RIGHT:
        break;
      default:
        break;
      }
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);
    SDL_Rect fillRect = {screenWidth / 4, screenHeight / 4, screenWidth / 2,
                         screenHeight / 2};
    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
    SDL_RenderFillRect(renderer, &fillRect);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF);
    SDL_RenderDrawLine(renderer, 0, screenHeight / 2, screenWidth,
                       screenHeight / 2);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF);
    for (int i = 0; i < screenHeight; i += 4) {
      SDL_RenderDrawPoint(renderer, screenWidth / 2, i);
    }

    SDL_RenderPresent(renderer);
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}