#ifndef SBASH64_GAME_SDL_WRAPPERS_HPP_
#define SBASH64_GAME_SDL_WRAPPERS_HPP_

#include "SDL_image.h"
#include <SDL.h>

#include <string>

namespace sbash64::game::sdl_wrappers {
struct Init {
  Init();
  ~Init();
};

struct Window {
  Window(int width, int height);
  ~Window();

  SDL_Window *window;
};

struct Renderer {
  explicit Renderer(SDL_Window *);
  ~Renderer();

  SDL_Renderer *renderer;
};

struct Texture {
  Texture(SDL_Renderer *, SDL_Surface *);
  ~Texture();

  SDL_Texture *texture;
};

struct ImageInit {
  ImageInit();
  ~ImageInit();
};

struct ImageSurface {
  explicit ImageSurface(const std::string &imagePath);
  ~ImageSurface();

  SDL_Surface *surface;
};
} // namespace sbash64::game::sdl_wrappers

#endif
