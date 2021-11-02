#ifndef SBASH64_GAME_SDL_WRAPPERS_HPP_
#define SBASH64_GAME_SDL_WRAPPERS_HPP_

#include "SDL_image.h"
#include <SDL.h>

#include <string>

namespace sbash64::game::sdl_wrappers {
struct Init {
  Init();
  ~Init();

  Init(Init &&) = delete;
  auto operator=(Init &&) -> Init & = delete;
  Init(const Init &) = delete;
  auto operator=(const Init &) -> Init & = delete;
};

struct Window {
  Window(int width, int height);
  ~Window();

  Window(Window &&) = delete;
  auto operator=(Window &&) -> Window & = delete;
  Window(const Window &) = delete;
  auto operator=(const Window &) -> Window & = delete;

  SDL_Window *window;
};

struct Renderer {
  explicit Renderer(SDL_Window *);
  ~Renderer();

  Renderer(Renderer &&) = delete;
  auto operator=(Renderer &&) -> Renderer & = delete;
  Renderer(const Renderer &) = delete;
  auto operator=(const Renderer &) -> Renderer & = delete;

  SDL_Renderer *renderer;
};

struct Texture {
  Texture(SDL_Renderer *, SDL_Surface *);
  ~Texture();

  Texture(Texture &&) = delete;
  auto operator=(Texture &&) -> Texture & = delete;
  Texture(const Texture &) = delete;
  auto operator=(const Texture &) -> Texture & = delete;

  SDL_Texture *texture;
};

struct ImageInit {
  ImageInit();
  ~ImageInit();

  ImageInit(ImageInit &&) = delete;
  auto operator=(ImageInit &&) -> ImageInit & = delete;
  ImageInit(const ImageInit &) = delete;
  auto operator=(const ImageInit &) -> ImageInit & = delete;
};

struct ImageSurface {
  explicit ImageSurface(const std::string &imagePath);
  ~ImageSurface();

  ImageSurface(ImageSurface &&) = delete;
  auto operator=(ImageSurface &&) -> ImageSurface & = delete;
  ImageSurface(const ImageSurface &) = delete;
  auto operator=(const ImageSurface &) -> ImageSurface & = delete;

  SDL_Surface *surface;
};
} // namespace sbash64::game::sdl_wrappers

#endif
