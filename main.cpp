#include "SDL_surface.h"
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_image.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sbash64::game {
constexpr auto screenWidth{640};
constexpr auto screenHeight{480};

enum class Color { red, blue, green, white };

static void throwRuntimeError(std::string_view message) {
  std::stringstream stream;
  stream << message << " SDL_Error: " << SDL_GetError();
  throw std::runtime_error{stream.str()};
}

namespace sdl_wrappers {
struct Init {
  Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
      throwRuntimeError("SDL could not initialize!");
  }

  ~Init() { SDL_Quit(); }
};

struct Window {
  Window()
      : window{SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED, screenWidth,
                                screenHeight, SDL_WINDOW_SHOWN)} {
    if (window == nullptr)
      throwRuntimeError("Window could not be created!");
  }

  ~Window() { SDL_DestroyWindow(window); }

  SDL_Window *window;
};

struct Renderer {
  explicit Renderer(SDL_Window *window)
      : renderer{SDL_CreateRenderer(
            window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)} {
    if (renderer == nullptr)
      throwRuntimeError("Renderer could not be created!");
  }

  ~Renderer() { SDL_DestroyRenderer(renderer); }

  SDL_Renderer *renderer;
};

struct Texture {
  Texture(SDL_Renderer *renderer, SDL_Surface *surface)
      : texture{SDL_CreateTextureFromSurface(renderer, surface)} {
    if (texture == nullptr)
      throwRuntimeError("Unable to create texture!");
  }

  ~Texture() { SDL_DestroyTexture(texture); }

  SDL_Texture *texture;
};

struct ImageSurface {
  explicit ImageSurface(const std::string &imagePath)
      : surface{IMG_Load(imagePath.c_str())} {
    if (surface == nullptr) {
      std::stringstream stream;
      stream << "Unable to load image " << imagePath << "!";
      throwRuntimeError(stream.str());
    }
  }

  ~ImageSurface() {}

  SDL_Surface *surface;
};
} // namespace sdl_wrappers

auto run(const std::string &imagePath) -> int {
  sdl_wrappers::Init scopedInitialization;
  sdl_wrappers::Window windowWrapper;
  sdl_wrappers::Renderer rendererWrapper{windowWrapper.window};
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

  SDL_SetRenderDrawColor(rendererWrapper.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  constexpr auto imageFlags{IMG_INIT_PNG};
  if (!(IMG_Init(imageFlags) & imageFlags)) {
    std::stringstream stream;
    stream << "SDL_image could not initialize! SDL_image Error: "
           << IMG_GetError();
    throw std::runtime_error{stream.str()};
  }

  sdl_wrappers::ImageSurface imageSurfaceWrapper{imagePath};
  SDL_SetColorKey(
      imageSurfaceWrapper.surface, SDL_TRUE,
      SDL_MapRGB(imageSurfaceWrapper.surface->format, 0, 0xFF, 0xFF));

  sdl_wrappers::Texture textureWrapper{rendererWrapper.renderer,
                                       imageSurfaceWrapper.surface};
  const auto imageWidth{imageSurfaceWrapper.surface->w};
  const auto imageHeight{imageSurfaceWrapper.surface->h};

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
    SDL_SetRenderDrawColor(rendererWrapper.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(rendererWrapper.renderer);
    SDL_Rect renderQuad = {x, y, imageWidth, imageHeight};
    SDL_RenderCopyEx(rendererWrapper.renderer, textureWrapper.texture, nullptr,
                     &renderQuad, 0, nullptr, SDL_FLIP_NONE);
    SDL_RenderPresent(rendererWrapper.renderer);
  }
  return EXIT_SUCCESS;
}
} // namespace sbash64::game

int main(int argc, char *argv[]) {
  if (argc < 2)
    return EXIT_FAILURE;
  try {
    return sbash64::game::run(argv[1]);
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}