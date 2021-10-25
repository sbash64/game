#include <sbash64/game/sdl-wrappers.hpp>

#include <SDL_image.h>

#include <sstream>
#include <stdexcept>
#include <string_view>

namespace sbash64::game {
[[noreturn]] static void throwRuntimeError(std::string_view message) {
  std::stringstream stream;
  stream << message << " SDL_Error: " << SDL_GetError();
  throw std::runtime_error{stream.str()};
}

namespace sdl_wrappers {
Init::Init() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    throwRuntimeError("SDL could not initialize!");
}

Init::~Init() { SDL_Quit(); }

Window::Window(int width, int height)
    : window{SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, width, height,
                              SDL_WINDOW_SHOWN)} {
  if (window == nullptr)
    throwRuntimeError("Window could not be created!");
}

Window::~Window() { SDL_DestroyWindow(window); }

Renderer::Renderer(SDL_Window *window)
    : renderer{SDL_CreateRenderer(
          window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)} {
  if (renderer == nullptr)
    throwRuntimeError("Renderer could not be created!");
}

Renderer::~Renderer() { SDL_DestroyRenderer(renderer); }

Texture::Texture(SDL_Renderer *renderer, SDL_Surface *surface)
    : texture{SDL_CreateTextureFromSurface(renderer, surface)} {
  if (texture == nullptr)
    throwRuntimeError("Unable to create texture!");
}

Texture::~Texture() { SDL_DestroyTexture(texture); }

ImageSurface::ImageSurface(const std::string &imagePath)
    : surface{IMG_Load(imagePath.c_str())} {
  if (surface == nullptr) {
    std::stringstream stream;
    stream << "Unable to load image " << imagePath << "!";
    throwRuntimeError(stream.str());
  }
}

ImageSurface::~ImageSurface() { SDL_FreeSurface(surface); }
} // namespace sdl_wrappers
} // namespace sbash64::game
