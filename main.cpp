#include <SDL.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_image.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

constexpr auto screenWidth{640};
constexpr auto screenHeight{480};

enum class Color { red, blue, green, white };

int main(int argc, char *argv[]) {
  if (argc < 2)
    return EXIT_FAILURE;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
              << '\n';
    return EXIT_FAILURE;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  auto *window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED, screenWidth,
                                  screenHeight, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError()
              << '\n';
    return EXIT_FAILURE;
  }

  auto *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    std::cerr << "Renderer could not be created! SDL Error: " << SDL_GetError()
              << '\n';
    return EXIT_FAILURE;
  }
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  auto imageFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imageFlags) & imageFlags)) {
    std::cerr << "SDL_image could not initialize! SDL_image Error: "
              << IMG_GetError() << '\n';
    return 1;
  }

  std::string path{argv[1]};
  auto *surface = IMG_Load(path.c_str());
  if (surface == nullptr) {
    std::cerr << "Unable to load image %s! SDL_image Error: " << path
              << IMG_GetError() << '\n';
    return EXIT_FAILURE;
  }
  SDL_SetColorKey(surface, SDL_TRUE,
                  SDL_MapRGB(surface->format, 0, 0xFF, 0xFF));

  auto *texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (texture == nullptr) {
    std::cerr << "Unable to create texture from %s! SDL Error: " << path
              << SDL_GetError() << '\n';
    return EXIT_FAILURE;
  }
  const auto imageWidth{surface->w};
  const auto imageHeight{surface->h};

  SDL_FreeSurface(surface);

  auto running{true};
  auto x{15};
  auto y{15};
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
      running = event.type != SDL_QUIT;
    if (event.type == SDL_KEYDOWN)
      switch (event.key.keysym.sym) {
      case SDLK_UP:
        --y;
        break;
      case SDLK_DOWN:
        ++y;
        break;
      case SDLK_LEFT:
        --x;
        break;
      case SDLK_RIGHT:
        ++x;
        break;
      default:
        break;
      }
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);
    SDL_Rect renderQuad = {x, y, imageWidth, imageHeight};
    SDL_RenderCopyEx(renderer, texture, nullptr, &renderQuad, 0, nullptr,
                     SDL_FLIP_NONE);
    SDL_RenderPresent(renderer);
  }
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}