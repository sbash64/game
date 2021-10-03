#include "SDL_scancode.h"
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_image.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sbash64::game {
struct RationalNumber {
  int numerator;
  int denominator;

  auto operator==(const RationalNumber &) const -> bool = default;
};

constexpr auto operator+=(RationalNumber &a, RationalNumber b)
    -> RationalNumber & {
  const auto smallerDenominator{std::min(a.denominator, b.denominator)};
  const auto largerDenominator{std::max(a.denominator, b.denominator)};
  auto commonDenominator = smallerDenominator;
  auto candidateDenominator = largerDenominator;
  while (true) {
    while (commonDenominator <
           std::min(std::numeric_limits<int>::max() - smallerDenominator,
                    candidateDenominator))
      commonDenominator += smallerDenominator;
    if (commonDenominator != candidateDenominator) {
      if (candidateDenominator <
          std::numeric_limits<int>::max() - largerDenominator)
        candidateDenominator += largerDenominator;
      else
        return a;
    } else {
      a.numerator = a.numerator * commonDenominator / a.denominator +
                    b.numerator * commonDenominator / b.denominator;
      a.denominator = commonDenominator;
      return a;
    }
  }
}

constexpr auto operator+(RationalNumber a, RationalNumber b) -> RationalNumber {
  return a += b;
}

constexpr auto round(RationalNumber a) -> int {
  return a.numerator / a.denominator +
         (a.numerator % a.denominator >= (a.denominator + 1) / 2 ? 1 : 0);
}

static_assert(RationalNumber{3, 4} + RationalNumber{5, 6} ==
                  RationalNumber{19, 12},
              "rational number arithmetic error");

static_assert(RationalNumber{4, 7} + RationalNumber{2, 3} ==
                  RationalNumber{26, 21},
              "rational number arithmetic error");

static_assert(round(RationalNumber{19, 12}) == 2,
              "rational number round error");

static_assert(round(RationalNumber{3, 7}) == 0, "rational number round error");

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

  ~ImageSurface() { SDL_FreeSurface(surface); }

  SDL_Surface *surface;
};
} // namespace sdl_wrappers

enum class JumpState { grounded, started, released };

static auto pressing(const Uint8 *keyStates, SDL_Scancode code) -> bool {
  return keyStates[code] != 0U;
}

auto run(const std::string &imagePath) -> int {
  sdl_wrappers::Init scopedInitialization;
  sdl_wrappers::Window windowWrapper;
  sdl_wrappers::Renderer rendererWrapper{windowWrapper.window};
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

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

  auto playing{true};
  auto x{0};
  auto y{screenHeight - imageHeight};
  auto horizontalVelocity{0};
  RationalNumber verticalVelocity{0, 1};
  auto jumpState{JumpState::grounded};
  RationalNumber gravity{1, 2};
  auto groundFriction{1};
  auto maxHorizontalSpeed{6};
  RationalNumber jumpAcceleration{-10, 1};
  auto runAcceleration{2};
  while (playing) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
      playing = event.type != SDL_QUIT;
    const auto *keyStates{SDL_GetKeyboardState(nullptr)};
    if (pressing(keyStates, SDL_SCANCODE_LEFT))
      horizontalVelocity -= runAcceleration;
    if (pressing(keyStates, SDL_SCANCODE_RIGHT))
      horizontalVelocity += runAcceleration;
    if (pressing(keyStates, SDL_SCANCODE_UP) &&
        jumpState == JumpState::grounded) {
      jumpState = JumpState::started;
      verticalVelocity += jumpAcceleration;
    }
    verticalVelocity += gravity;
    if (!pressing(keyStates, SDL_SCANCODE_UP) &&
        jumpState == JumpState::started) {
      jumpState = JumpState::released;
      if (verticalVelocity.numerator < 0)
        verticalVelocity = {0, 1};
    }
    if (horizontalVelocity > maxHorizontalSpeed)
      horizontalVelocity = maxHorizontalSpeed;
    if (horizontalVelocity < -maxHorizontalSpeed)
      horizontalVelocity = -maxHorizontalSpeed;
    if (horizontalVelocity > 0)
      horizontalVelocity -= groundFriction;
    if (horizontalVelocity < 0)
      horizontalVelocity += groundFriction;
    if (round(verticalVelocity) + y > screenHeight - imageHeight) {
      verticalVelocity = {0, 1};
      y = screenHeight - imageHeight;
      jumpState = JumpState::grounded;
    }
    x += horizontalVelocity;
    y += round(verticalVelocity);
    SDL_SetRenderDrawColor(rendererWrapper.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(rendererWrapper.renderer);
    const SDL_Rect renderQuad{x, y, imageWidth, imageHeight};
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