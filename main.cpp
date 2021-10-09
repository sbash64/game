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

constexpr auto operator+(RationalNumber a, int b) -> RationalNumber {
  return RationalNumber{a.numerator + a.denominator * b, a.denominator};
}

constexpr auto operator+(RationalNumber a, RationalNumber b) -> RationalNumber {
  return a += b;
}

constexpr auto operator-(RationalNumber a) -> RationalNumber {
  return {-a.numerator, a.denominator};
}

constexpr auto operator/(RationalNumber a, RationalNumber b) -> RationalNumber {
  return {a.numerator * b.denominator, a.denominator * b.numerator};
}

constexpr auto operator/(RationalNumber a, int b) -> RationalNumber {
  return {a.numerator, a.denominator * b};
}

constexpr auto operator<(RationalNumber a, RationalNumber b) -> bool {
  return (a.denominator < 0 ^ b.denominator < 0) != 0
             ? a.numerator * b.denominator > b.numerator * a.denominator
             : a.numerator * b.denominator < b.numerator * a.denominator;
}

constexpr auto operator>(RationalNumber a, RationalNumber b) -> bool {
  return b < a;
}

constexpr auto operator<=(RationalNumber a, RationalNumber b) -> bool {
  return !(a > b);
}

constexpr auto operator>=(RationalNumber a, RationalNumber b) -> bool {
  return !(a < b);
}

constexpr auto operator<(RationalNumber a, int b) -> bool {
  return a.denominator < 0 ? a.numerator > b * a.denominator
                           : a.numerator < b * a.denominator;
}

constexpr auto operator>(RationalNumber a, int b) -> bool {
  return a.denominator < 0 ? a.numerator < b * a.denominator
                           : a.numerator > b * a.denominator;
}

constexpr auto absoluteValue(int a) -> int { return a < 0 ? -a : a; }

constexpr auto round(RationalNumber a) -> int {
  const auto division{a.numerator / a.denominator};
  if (absoluteValue(a.numerator) % a.denominator <
      (absoluteValue(a.denominator) + 1) / 2)
    return division;
  return (a.numerator < 0 ^ a.denominator < 0) != 0 ? division - 1
                                                    : division + 1;
}

static_assert(7 % 4 == 3, "do I understand cpp modulus?");
static_assert(7 % -4 == 3, "do I understand cpp modulus?");
static_assert(-7 % 4 == -3, "do I understand cpp modulus?");
static_assert(-7 % -4 == -3, "do I understand cpp modulus?");

static_assert(RationalNumber{3, 4} + RationalNumber{5, 6} ==
                  RationalNumber{19, 12},
              "rational number arithmetic error");

static_assert(RationalNumber{4, 7} + RationalNumber{2, 3} ==
                  RationalNumber{26, 21},
              "rational number arithmetic error");

static_assert(RationalNumber{4, 7} + 3 == RationalNumber{25, 7},
              "rational number arithmetic error");

static_assert(RationalNumber{4, 7} / RationalNumber{2, 3} ==
                  RationalNumber{12, 14},
              "rational number arithmetic error");

static_assert(RationalNumber{4, 7} / 2 == RationalNumber{4, 14},
              "rational number arithmetic error");

static_assert(RationalNumber{19, 12} < RationalNumber{7, 3},
              "rational number comparison error");

static_assert(RationalNumber{-1, 2} < RationalNumber{1, 3},
              "rational number comparison error");

static_assert(RationalNumber{-1, 2} < RationalNumber{1, -3},
              "rational number comparison error");

static_assert(RationalNumber{1, -2} < RationalNumber{-1, 3},
              "rational number comparison error");

static_assert(RationalNumber{1, -2} < RationalNumber{-1, -3},
              "rational number comparison error");

static_assert(RationalNumber{-1, -2} > RationalNumber{-1, -3},
              "rational number comparison error");

static_assert(RationalNumber{2, 3} > RationalNumber{1, 4},
              "rational number comparison error");

static_assert(RationalNumber{-2, 3} > -1, "rational number comparison error");

static_assert(RationalNumber{-2, 3} < 0, "rational number comparison error");

static_assert(RationalNumber{2, -3} < 0, "rational number comparison error");

static_assert(RationalNumber{-2, -3} > 0, "rational number comparison error");

static_assert(RationalNumber{2, 3} > RationalNumber{1, 4},
              "rational number comparison error");

static_assert(round(RationalNumber{19, 12}) == 2,
              "rational number round error");

static_assert(round(RationalNumber{3, 7}) == 0, "rational number round error");

static_assert(round(RationalNumber{-3, 7}) == 0, "rational number round error");

static_assert(round(RationalNumber{-4, 7}) == -1,
              "rational number round error");

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

struct Velocity {
  RationalNumber vertical;
  int horizontal;
};

constexpr auto operator-(Velocity a) -> Velocity {
  return {-a.vertical, -a.horizontal};
}

constexpr auto applyHorizontalVelocity(Rectangle a, Velocity b) -> Rectangle {
  a.origin.x += b.horizontal;
  return a;
}

constexpr auto applyVerticalVelocity(Rectangle a, Velocity b) -> Rectangle {
  a.origin.y += round(b.vertical);
  return a;
}

constexpr auto topEdge(Rectangle a) -> int { return a.origin.y; }

constexpr auto leftEdge(Rectangle a) -> int { return a.origin.x; }

constexpr auto rightEdge(Rectangle a) -> int {
  return a.origin.x + a.width - 1;
}

constexpr auto bottomEdge(Rectangle a) -> int {
  return a.origin.y + a.height - 1;
}

constexpr auto operator*=(Rectangle &a, int scale) -> Rectangle & {
  a.origin.x *= scale;
  a.origin.y *= scale;
  a.width *= scale;
  a.height *= scale;
  return a;
}

constexpr auto operator*(Rectangle a, int scale) -> Rectangle {
  return a *= scale;
}

constexpr auto distanceFirstExceedsSecondVertically(Rectangle a, Rectangle b)
    -> int {
  return bottomEdge(a) - topEdge(b);
}

constexpr auto distanceFirstExceedsSecondHorizontally(Rectangle a, Rectangle b)
    -> int {
  return rightEdge(a) - leftEdge(b);
}

constexpr auto clamp(int velocity, int limit) -> int {
  return std::clamp(velocity, -limit, limit);
}

constexpr auto withFriction(int velocity, int friction) -> int {
  return ((velocity < 0) ? -1 : 1) * std::max(0, std::abs(velocity) - friction);
}

constexpr auto isNonnegative(int a) -> bool { return a >= 0; }

class Collision {
public:
  [[nodiscard]] virtual auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> int = 0;
  [[nodiscard]] virtual auto
  distanceFirstExceedsSecondNormalToSurface(Rectangle a, Rectangle b) const
      -> int = 0;
  [[nodiscard]] virtual auto applyVelocityNormalToSurface(Rectangle a,
                                                          Velocity b) const
      -> Rectangle = 0;
  [[nodiscard]] virtual auto applyVelocityParallelToSurface(Rectangle a,
                                                            Velocity b) const
      -> Rectangle = 0;
  [[nodiscard]] virtual auto combinedVelocity(Velocity a) const
      -> RationalNumber = 0;
  [[nodiscard]] virtual auto headingTowardUpperBoundary(Velocity a) const
      -> bool = 0;
  [[nodiscard]] virtual auto headingTowardLowerBoundary(Velocity a) const
      -> bool = 0;
};

class CollisionFromBelow : public Collision {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondHorizontally(a, b);
  }

  [[nodiscard]] auto
  distanceFirstExceedsSecondNormalToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondVertically(a, b);
  }

  [[nodiscard]] auto applyVelocityNormalToSurface(Rectangle a, Velocity b) const
      -> Rectangle override {
    return applyVerticalVelocity(a, b);
  }

  [[nodiscard]] auto applyVelocityParallelToSurface(Rectangle a,
                                                    Velocity b) const
      -> Rectangle override {
    return applyHorizontalVelocity(a, b);
  }

  [[nodiscard]] auto combinedVelocity(Velocity a) const
      -> RationalNumber override {
    return RationalNumber{round(a.vertical), a.horizontal};
  }

  [[nodiscard]] auto headingTowardUpperBoundary(Velocity a) const
      -> bool override {
    return a.horizontal > 0;
  }

  [[nodiscard]] auto headingTowardLowerBoundary(Velocity a) const
      -> bool override {
    return a.horizontal < 0;
  }
};

class CollisionFromAbove : public Collision {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondHorizontally(a, b);
  }

  [[nodiscard]] auto
  distanceFirstExceedsSecondNormalToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondVertically(b, a);
  }

  [[nodiscard]] auto applyVelocityNormalToSurface(Rectangle a, Velocity b) const
      -> Rectangle override {
    return applyVerticalVelocity(a, b);
  }

  [[nodiscard]] auto applyVelocityParallelToSurface(Rectangle a,
                                                    Velocity b) const
      -> Rectangle override {
    return applyHorizontalVelocity(a, b);
  }

  [[nodiscard]] auto combinedVelocity(Velocity a) const
      -> RationalNumber override {
    return RationalNumber{-round(a.vertical), a.horizontal};
  }

  [[nodiscard]] auto headingTowardUpperBoundary(Velocity a) const
      -> bool override {
    return a.horizontal > 0;
  }

  [[nodiscard]] auto headingTowardLowerBoundary(Velocity a) const
      -> bool override {
    return a.horizontal < 0;
  }
};

class CollisionFromRight : public Collision {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondVertically(a, b);
  }

  [[nodiscard]] auto
  distanceFirstExceedsSecondNormalToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondHorizontally(a, b);
  }

  [[nodiscard]] auto applyVelocityNormalToSurface(Rectangle a, Velocity b) const
      -> Rectangle override {
    return applyHorizontalVelocity(a, b);
  }

  [[nodiscard]] auto applyVelocityParallelToSurface(Rectangle a,
                                                    Velocity b) const
      -> Rectangle override {
    return applyVerticalVelocity(a, b);
  }

  [[nodiscard]] auto combinedVelocity(Velocity a) const
      -> RationalNumber override {
    return RationalNumber{a.horizontal, round(a.vertical)};
  }

  [[nodiscard]] auto headingTowardUpperBoundary(Velocity a) const
      -> bool override {
    return round(a.vertical) > 0;
  }

  [[nodiscard]] auto headingTowardLowerBoundary(Velocity a) const
      -> bool override {
    return round(a.vertical) < 0;
  }
};

class CollisionFromLeft : public Collision {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondVertically(a, b);
  }

  [[nodiscard]] auto
  distanceFirstExceedsSecondNormalToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondHorizontally(b, a);
  }

  [[nodiscard]] auto applyVelocityNormalToSurface(Rectangle a, Velocity b) const
      -> Rectangle override {
    return applyHorizontalVelocity(a, b);
  }

  [[nodiscard]] auto applyVelocityParallelToSurface(Rectangle a,
                                                    Velocity b) const
      -> Rectangle override {
    return applyVerticalVelocity(a, b);
  }

  [[nodiscard]] auto combinedVelocity(Velocity a) const
      -> RationalNumber override {
    return RationalNumber{-a.horizontal, round(a.vertical)};
  }

  [[nodiscard]] auto headingTowardUpperBoundary(Velocity a) const
      -> bool override {
    return round(a.vertical) > 0;
  }

  [[nodiscard]] auto headingTowardLowerBoundary(Velocity a) const
      -> bool override {
    return round(a.vertical) < 0;
  }
};

static auto playerPassesThrough(Rectangle playerRectangle,
                                Rectangle wallRectangle,
                                Velocity playerVelocity,
                                const Collision &collision) -> bool {
  const auto playerDoesNotExceedWallsUpperBoundary =
      isNonnegative(collision.distanceFirstExceedsSecondParallelToSurface(
          wallRectangle, playerRectangle));
  const auto playerDoesNotPrecedeWallsLowerBoundary =
      isNonnegative(collision.distanceFirstExceedsSecondParallelToSurface(
          playerRectangle, wallRectangle));
  const auto playerPassesThroughWallTowardUpperBoundary{
      collision.headingTowardUpperBoundary(playerVelocity) &&
      isNonnegative(collision.distanceFirstExceedsSecondParallelToSurface(
          collision.applyVelocityParallelToSurface(playerRectangle,
                                                   playerVelocity),
          wallRectangle)) &&
      playerDoesNotExceedWallsUpperBoundary &&
      collision.combinedVelocity(playerVelocity) >
          RationalNumber{-(collision.distanceFirstExceedsSecondNormalToSurface(
                               playerRectangle, wallRectangle) +
                           1),
                         collision.distanceFirstExceedsSecondParallelToSurface(
                             wallRectangle, playerRectangle) +
                             1}};
  const auto playerPassesThroughWallTowardLowerBoundary{
      collision.headingTowardLowerBoundary(playerVelocity) &&
      isNonnegative(collision.distanceFirstExceedsSecondParallelToSurface(
          wallRectangle, collision.applyVelocityParallelToSurface(
                             playerRectangle, playerVelocity))) &&
      playerDoesNotPrecedeWallsLowerBoundary &&
      collision.combinedVelocity(playerVelocity) <
          RationalNumber{collision.distanceFirstExceedsSecondNormalToSurface(
                             playerRectangle, wallRectangle) +
                             1,
                         collision.distanceFirstExceedsSecondParallelToSurface(
                             playerRectangle, wallRectangle) +
                             1}};
  const auto playerPassesThroughWallDirectly{
      !collision.headingTowardUpperBoundary(playerVelocity) &&
      !collision.headingTowardLowerBoundary(playerVelocity) &&
      playerDoesNotPrecedeWallsLowerBoundary &&
      playerDoesNotExceedWallsUpperBoundary};
  return playerPassesThroughWallTowardLowerBoundary ||
         playerPassesThroughWallTowardUpperBoundary ||
         playerPassesThroughWallDirectly;
}

constexpr auto pixelScale{4};
const auto cameraWidth{256};
const auto cameraHeight{240};
constexpr auto screenWidth{cameraWidth * pixelScale};
constexpr auto screenHeight{cameraHeight * pixelScale};

static void onPlayerHitGround(RationalNumber &playerVerticalVelocity,
                              Rectangle &playerRectangle,
                              JumpState &playerJumpState, int ground) {
  playerVerticalVelocity = {0, 1};
  playerRectangle.origin.y = ground - playerRectangle.height;
  playerJumpState = JumpState::grounded;
}

[[noreturn]] static void throwRuntimeError(std::string_view message) {
  std::stringstream stream;
  stream << message << " SDL_Error: " << SDL_GetError();
  throw std::runtime_error{stream.str()};
}

namespace sdl_wrappers {
namespace {
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
} // namespace
} // namespace sdl_wrappers

constexpr auto toSDLRect(Rectangle a) -> SDL_Rect {
  SDL_Rect converted;
  converted.x = a.origin.x;
  converted.y = a.origin.y;
  converted.w = a.width;
  converted.h = a.height;
  return converted;
}

static auto pressing(const Uint8 *keyStates, SDL_Scancode code) -> bool {
  return keyStates[code] != 0U;
}

static void initializeSDLImage() {
  constexpr auto imageFlags{IMG_INIT_PNG};
  if (!(IMG_Init(imageFlags) & imageFlags)) {
    std::stringstream stream;
    stream << "SDL_image could not initialize! SDL_image Error: "
           << IMG_GetError();
    throw std::runtime_error{stream.str()};
  }
}

static auto run(const std::string &playerImagePath,
                const std::string &backgroundImagePath) -> int {
  sdl_wrappers::Init scopedInitialization;
  sdl_wrappers::Window windowWrapper;
  sdl_wrappers::Renderer rendererWrapper{windowWrapper.window};
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  initializeSDLImage();
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
  const RationalNumber gravity{1, 4};
  const auto groundFriction{1};
  const auto playerMaxHorizontalSpeed{4};
  const RationalNumber playerJumpAcceleration{-5, 1};
  const auto playerRunAcceleration{2};
  Rectangle blockRectangle{Point{256, 144}, 40, 40};
  Rectangle backgroundSourceRect{Point{0, 0}, cameraWidth, cameraHeight};
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
        withFriction(clamp(playerHorizontalVelocity, playerMaxHorizontalSpeed),
                     groundFriction);
    const auto playerIsAboveTopOfWall =
        distanceFirstExceedsSecondVertically(playerRectangle, blockRectangle) <
        0;
    const auto playerIsBelowBottomOfWall =
        distanceFirstExceedsSecondVertically(blockRectangle, playerRectangle) <
        0;
    const auto playerWillBeBelowTopOfWall =
        isNonnegative(distanceFirstExceedsSecondVertically(
            applyVerticalVelocity(playerRectangle, {playerVerticalVelocity,
                                                    playerHorizontalVelocity}),
            blockRectangle));
    const auto playerWillBeAboveBottomOfWall =
        isNonnegative(distanceFirstExceedsSecondVertically(
            blockRectangle, applyVerticalVelocity(playerRectangle,
                                                  {playerVerticalVelocity,
                                                   playerHorizontalVelocity})));
    if (playerIsAboveTopOfWall && playerWillBeBelowTopOfWall &&
        playerPassesThrough(playerRectangle, blockRectangle,
                            {playerVerticalVelocity, playerHorizontalVelocity},
                            CollisionFromBelow{}))
      onPlayerHitGround(playerVerticalVelocity, playerRectangle,
                        playerJumpState, topEdge(blockRectangle));
    else if (playerIsBelowBottomOfWall && playerWillBeAboveBottomOfWall &&
             playerPassesThrough(
                 playerRectangle, blockRectangle,
                 {playerVerticalVelocity, playerHorizontalVelocity},
                 CollisionFromAbove{})) {
      playerVerticalVelocity = {0, 1};
      playerRectangle.origin.y = bottomEdge(blockRectangle) + 1;
    } else if (bottomEdge(applyVerticalVelocity(
                   playerRectangle, {playerVerticalVelocity,
                                     playerHorizontalVelocity})) >= ground)
      onPlayerHitGround(playerVerticalVelocity, playerRectangle,
                        playerJumpState, ground);

    const auto playerIsBeforeLeftOfWall =
        distanceFirstExceedsSecondHorizontally(playerRectangle,
                                               blockRectangle) < 0;
    const auto playerIsAfterRightOfWall =
        distanceFirstExceedsSecondHorizontally(blockRectangle,
                                               playerRectangle) < 0;
    const auto playerWillBeAheadOfLeftOfWall =
        isNonnegative(distanceFirstExceedsSecondHorizontally(
            applyHorizontalVelocity(
                playerRectangle,
                {playerVerticalVelocity, playerHorizontalVelocity}),
            blockRectangle));
    const auto playerWillBeBeforeRightOfWall =
        isNonnegative(distanceFirstExceedsSecondHorizontally(
            blockRectangle, applyHorizontalVelocity(
                                playerRectangle, {playerVerticalVelocity,
                                                  playerHorizontalVelocity})));
    if (playerIsBeforeLeftOfWall && playerWillBeAheadOfLeftOfWall &&
        playerPassesThrough(playerRectangle, blockRectangle,
                            {playerVerticalVelocity, playerHorizontalVelocity},
                            CollisionFromRight{})) {
      playerHorizontalVelocity = 0;
      playerRectangle.origin.x =
          leftEdge(blockRectangle) - playerRectangle.width;
    } else if (playerIsAfterRightOfWall && playerWillBeBeforeRightOfWall &&
               playerPassesThrough(
                   playerRectangle, blockRectangle,
                   {playerVerticalVelocity, playerHorizontalVelocity},
                   CollisionFromLeft{})) {
      playerHorizontalVelocity = 0;
      playerRectangle.origin.x = rightEdge(blockRectangle) + 1;
    }
    playerRectangle.origin.x += playerHorizontalVelocity;
    playerRectangle = applyVerticalVelocity(
        playerRectangle, {playerVerticalVelocity, playerHorizontalVelocity});
    const auto playerDistanceRightOfCenter{
        playerRectangle.origin.x + playerRectangle.width / 2 - cameraWidth / 2};
    const auto distanceFromBackgroundRightEdgeToEnd{
        backgroundSourceWidth - rightEdge(backgroundSourceRect) - 1};
    if (playerDistanceRightOfCenter > 0 &&
        distanceFromBackgroundRightEdgeToEnd > 0) {
      const auto shift{std::min(distanceFromBackgroundRightEdgeToEnd,
                                playerDistanceRightOfCenter)};
      backgroundSourceRect.origin.x += shift;
      blockRectangle.origin.x -= shift;
      playerRectangle.origin.x -= shift;
    } else if (playerDistanceRightOfCenter < 0 &&
               leftEdge(backgroundSourceRect) > 0) {
      const auto shift{std::max(-leftEdge(backgroundSourceRect),
                                playerDistanceRightOfCenter)};
      backgroundSourceRect.origin.x += shift;
      blockRectangle.origin.x -= shift;
      playerRectangle.origin.x -= shift;
    }
    // SDL_SetRenderDrawColor(rendererWrapper.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    // SDL_RenderClear(rendererWrapper.renderer);
    const auto backgroundSourceRectConverted{toSDLRect(backgroundSourceRect)};
    const Rectangle backgroundRectangle{{0, 0}, cameraWidth, cameraHeight};
    const auto backgroundProjection{
        toSDLRect(backgroundRectangle * pixelScale)};
    SDL_RenderCopyEx(rendererWrapper.renderer, backgroundTextureWrapper.texture,
                     &backgroundSourceRectConverted, &backgroundProjection, 0,
                     nullptr, SDL_FLIP_NONE);
    SDL_SetRenderDrawColor(rendererWrapper.renderer, 0x00, 0x00, 0x00, 0xFF);
    const auto wallProjection{toSDLRect(blockRectangle * pixelScale)};
    SDL_RenderFillRect(rendererWrapper.renderer, &wallProjection);
    const auto playerProjection{toSDLRect(playerRectangle * pixelScale)};
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
