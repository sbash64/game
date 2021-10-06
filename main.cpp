#include <SDL.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_image.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_scancode.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

// https://stackoverflow.com/a/53067795
static auto getpixel(SDL_Surface *surface, int x, int y) -> Uint32 {
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  const auto *p = static_cast<const Uint8 *>(surface->pixels) +
                  static_cast<std::ptrdiff_t>(y) * surface->pitch +
                  static_cast<std::ptrdiff_t>(x) * bpp;
  switch (bpp) {
  case 1:
    return *p;
  case 2:
    return *reinterpret_cast<const Uint16 *>(p);
  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
      return p[0] << 16 | p[1] << 8 | p[2];
    else
      return p[0] | p[1] << 8 | p[2] << 16;
  case 4:
    return *reinterpret_cast<const Uint32 *>(p);
  default:
    return 0; /* shouldn't happen, but avoids warnings */
  }
}

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

constexpr auto pixelScale{4};
const auto cameraWidth{256};
const auto cameraHeight{240};
constexpr auto screenWidth{cameraWidth * pixelScale};
constexpr auto screenHeight{cameraHeight * pixelScale};

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

struct Point {
  int x;
  int y;
};

struct Rectangle {
  Point origin;
  int width;
  int height;
};

static auto pressing(const Uint8 *keyStates, SDL_Scancode code) -> bool {
  return keyStates[code] != 0U;
}

static void onPlayerHitGround(RationalNumber &playerVerticalVelocity,
                              Rectangle &playerRectangle,
                              JumpState &playerJumpState, int ground) {
  playerVerticalVelocity = {0, 1};
  playerRectangle.origin.y = ground - playerRectangle.height;
  playerJumpState = JumpState::grounded;
}

static auto run(const std::string &playerImagePath,
                const std::string &backgroundImagePath) -> int {
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

  sdl_wrappers::ImageSurface playerImageSurfaceWrapper{playerImagePath};
  SDL_SetColorKey(playerImageSurfaceWrapper.surface, SDL_TRUE,
                  getpixel(playerImageSurfaceWrapper.surface, 1, 9));
  const auto playerWidth{16};
  const auto playerHeight{16};
  const SDL_Rect playerSourceRect{1, 9, playerWidth, playerHeight};

  sdl_wrappers::ImageSurface backgroundImageSurfaceWrapper{backgroundImagePath};
  const auto backgroundSourceWidth{backgroundImageSurfaceWrapper.surface->w};
  const auto backgroundSourceHeight{backgroundImageSurfaceWrapper.surface->h};

  sdl_wrappers::Texture playerTextureWrapper{rendererWrapper.renderer,
                                             playerImageSurfaceWrapper.surface};

  sdl_wrappers::Texture backgroundTextureWrapper{
      rendererWrapper.renderer, backgroundImageSurfaceWrapper.surface};
  auto playing{true};
  const auto ground{cameraHeight - 32};
  Rectangle playerRectangle{Point{0, ground - playerHeight}, playerWidth,
                            playerHeight};
  auto playerHorizontalVelocity{0};
  RationalNumber playerVerticalVelocity{0, 1};
  auto playerJumpState{JumpState::grounded};
  RationalNumber gravity{1, 2};
  auto groundFriction{1};
  auto playerMaxHorizontalSpeed{4};
  RationalNumber playerJumpAcceleration{-10, 1};
  auto playerRunAcceleration{2};
  SDL_Rect wallRect{100, cameraHeight - 60, 50, 60};
  const SDL_Rect backgroundProjection{0, 0, screenWidth, screenHeight};
  SDL_Rect backgroundSourceRect{0, 0, cameraWidth, cameraHeight};
  while (playing) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
      playing = event.type != SDL_QUIT;
    const auto *keyStates{SDL_GetKeyboardState(nullptr)};
    if (pressing(keyStates, SDL_SCANCODE_LEFT))
      playerHorizontalVelocity -= playerRunAcceleration;
    if (pressing(keyStates, SDL_SCANCODE_RIGHT))
      playerHorizontalVelocity += playerRunAcceleration;
    if (pressing(keyStates, SDL_SCANCODE_UP) &&
        playerJumpState == JumpState::grounded) {
      playerJumpState = JumpState::started;
      playerVerticalVelocity += playerJumpAcceleration;
    }
    playerVerticalVelocity += gravity;
    if (!pressing(keyStates, SDL_SCANCODE_UP) &&
        playerJumpState == JumpState::started) {
      playerJumpState = JumpState::released;
      if (playerVerticalVelocity.numerator < 0)
        playerVerticalVelocity = {0, 1};
    }
    playerHorizontalVelocity =
        std::clamp(playerHorizontalVelocity, -playerMaxHorizontalSpeed,
                   playerMaxHorizontalSpeed);
    playerHorizontalVelocity =
        ((playerHorizontalVelocity < 0) ? -1 : 1) *
        std::max(0, std::abs(playerHorizontalVelocity) - groundFriction);
    const auto playerBottomEdge{playerRectangle.origin.y +
                                playerRectangle.height - 1};
    const auto playerRightEdge{playerRectangle.origin.x +
                               playerRectangle.width - 1};
    const auto wallLeftEdge{wallRect.x};
    const auto wallRightEdge{wallLeftEdge + wallRect.w - 1};
    const auto wallTopEdge{wallRect.y};
    const auto playerWillBeRightOfWallsLeftEdge{
        playerRightEdge + playerHorizontalVelocity >= wallLeftEdge};
    const auto playerWillBeLeftOfWallsRightEdge{
        playerRectangle.origin.x + playerHorizontalVelocity <= wallRightEdge};
    const auto playerWillBeBelowWallsTopEdge{
        playerBottomEdge + round(playerVerticalVelocity) >= wallTopEdge};
    const auto playerIsAboveWall{playerBottomEdge < wallTopEdge};
    const auto playerWillBeBelowGround{
        round(playerVerticalVelocity) + playerBottomEdge >= ground};
    if (playerIsAboveWall && playerWillBeBelowWallsTopEdge &&
        playerWillBeRightOfWallsLeftEdge && playerWillBeLeftOfWallsRightEdge)
      onPlayerHitGround(playerVerticalVelocity, playerRectangle,
                        playerJumpState, wallTopEdge);
    else if (playerWillBeBelowGround)
      onPlayerHitGround(playerVerticalVelocity, playerRectangle,
                        playerJumpState, ground);
    else
      playerRectangle.origin.y += round(playerVerticalVelocity);
    const auto playerIsLeftOfWall{playerRightEdge < wallLeftEdge};
    const auto playerIsRightOfWall{playerRectangle.origin.x > wallRightEdge};
    if (playerIsLeftOfWall && playerWillBeRightOfWallsLeftEdge &&
        playerWillBeBelowWallsTopEdge) {
      playerHorizontalVelocity = 0;
      playerRectangle.origin.x = wallLeftEdge - playerRectangle.width;
    } else if (playerIsRightOfWall && playerWillBeLeftOfWallsRightEdge &&
               playerWillBeBelowWallsTopEdge) {
      playerHorizontalVelocity = 0;
      playerRectangle.origin.x = wallRightEdge + 1;
    } else
      playerRectangle.origin.x += playerHorizontalVelocity;
    const auto playerDistanceRightOfCenter{
        playerRectangle.origin.x + playerRectangle.width / 2 - cameraWidth / 2};
    const auto backgroundLeftEdge{backgroundSourceRect.x};
    const auto backgroundRightEdge{backgroundLeftEdge + backgroundSourceRect.w -
                                   1};
    const auto distanceFromBackgroundRightEdgeToEnd{backgroundSourceWidth -
                                                    backgroundRightEdge - 1};
    if (playerDistanceRightOfCenter > 0 &&
        distanceFromBackgroundRightEdgeToEnd > 0) {
      const auto shift{std::min(distanceFromBackgroundRightEdgeToEnd,
                                playerDistanceRightOfCenter)};
      backgroundSourceRect.x += shift;
      wallRect.x -= shift;
      playerRectangle.origin.x -= shift;
    } else if (playerDistanceRightOfCenter < 0 && backgroundLeftEdge > 0) {
      const auto shift{
          std::max(-backgroundLeftEdge, playerDistanceRightOfCenter)};
      backgroundSourceRect.x += shift;
      wallRect.x -= shift;
      playerRectangle.origin.x -= shift;
    }
    // SDL_SetRenderDrawColor(rendererWrapper.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    // SDL_RenderClear(rendererWrapper.renderer);
    SDL_RenderCopyEx(rendererWrapper.renderer, backgroundTextureWrapper.texture,
                     &backgroundSourceRect, &backgroundProjection, 0, nullptr,
                     SDL_FLIP_NONE);
    SDL_SetRenderDrawColor(rendererWrapper.renderer, 0x00, 0x00, 0x00, 0xFF);
    const SDL_Rect wallProjection{
        wallRect.x * pixelScale, wallRect.y * pixelScale,
        wallRect.w * pixelScale, wallRect.h * pixelScale};
    SDL_RenderFillRect(rendererWrapper.renderer, &wallProjection);
    const SDL_Rect playerProjection{playerRectangle.origin.x * pixelScale,
                                    playerRectangle.origin.y * pixelScale,
                                    playerRectangle.width * pixelScale,
                                    playerRectangle.height * pixelScale};
    SDL_RenderCopyEx(rendererWrapper.renderer, playerTextureWrapper.texture,
                     &playerSourceRect, &playerProjection, 0, nullptr,
                     SDL_FLIP_NONE);
    SDL_RenderPresent(rendererWrapper.renderer);
  }
  return EXIT_SUCCESS;
}
} // namespace sbash64::game

int main(int argc, char *argv[]) {
  if (argc < 3)
    return EXIT_FAILURE;
  try {
    return sbash64::game::run(argv[1], argv[2]);
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}