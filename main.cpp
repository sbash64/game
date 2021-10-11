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
  const auto bpp{surface->format->BytesPerPixel};
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
    return 0;
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

constexpr auto operator+=(RationalNumber &a, int b) -> RationalNumber {
  a.numerator += a.denominator * b;
  return a;
}

constexpr auto operator+(RationalNumber a, int b) -> RationalNumber {
  return a += b;
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

static_assert(RationalNumber{4, 7} + RationalNumber{-2, 3} ==
                  RationalNumber{-2, 21},
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

class CollisionDirection {
public:
  [[nodiscard]] virtual auto distanceSubjectPenetratesObject(Rectangle a,
                                                             Rectangle b) const
      -> int = 0;
};

class CollisionAxis {
public:
  [[nodiscard]] virtual auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> int = 0;
  [[nodiscard]] virtual auto applyVelocityNormalToSurface(Rectangle a,
                                                          Velocity b) const
      -> Rectangle = 0;
  [[nodiscard]] virtual auto applyVelocityParallelToSurface(Rectangle a,
                                                            Velocity b) const
      -> Rectangle = 0;
  [[nodiscard]] virtual auto headingTowardUpperBoundary(Velocity a) const
      -> bool = 0;
  [[nodiscard]] virtual auto headingTowardLowerBoundary(Velocity a) const
      -> bool = 0;
  [[nodiscard]] virtual auto surfaceRelativeSlope(Velocity a) const
      -> RationalNumber = 0;
};

class HorizontalCollision : public CollisionAxis {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondVertically(a, b);
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

  [[nodiscard]] auto headingTowardUpperBoundary(Velocity a) const
      -> bool override {
    return round(a.vertical) > 0;
  }

  [[nodiscard]] auto headingTowardLowerBoundary(Velocity a) const
      -> bool override {
    return round(a.vertical) < 0;
  }

  [[nodiscard]] auto surfaceRelativeSlope(Velocity a) const
      -> RationalNumber override {
    return RationalNumber{std::abs(a.horizontal), round(a.vertical)};
  }
};

class VerticalCollision : public CollisionAxis {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> int override {
    return distanceFirstExceedsSecondHorizontally(a, b);
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

  [[nodiscard]] auto headingTowardUpperBoundary(Velocity a) const
      -> bool override {
    return a.horizontal > 0;
  }

  [[nodiscard]] auto headingTowardLowerBoundary(Velocity a) const
      -> bool override {
    return a.horizontal < 0;
  }

  [[nodiscard]] auto surfaceRelativeSlope(Velocity a) const
      -> RationalNumber override {
    return RationalNumber{std::abs(round(a.vertical)), a.horizontal};
  }
};

class CollisionFromBelow : public CollisionDirection {
public:
  [[nodiscard]] auto
  distanceSubjectPenetratesObject(Rectangle subjectRectangle,
                                  Rectangle objectRectangle) const
      -> int override {
    return distanceFirstExceedsSecondVertically(subjectRectangle,
                                                objectRectangle);
  }
};

class CollisionFromAbove : public CollisionDirection {
public:
  [[nodiscard]] auto
  distanceSubjectPenetratesObject(Rectangle subjectRectangle,
                                  Rectangle objectRectangle) const
      -> int override {
    return distanceFirstExceedsSecondVertically(objectRectangle,
                                                subjectRectangle);
  }
};

class CollisionFromRight : public CollisionDirection {
public:
  [[nodiscard]] auto
  distanceSubjectPenetratesObject(Rectangle subjectRectangle,
                                  Rectangle objectRectangle) const
      -> int override {
    return distanceFirstExceedsSecondHorizontally(subjectRectangle,
                                                  objectRectangle);
  }
};

class CollisionFromLeft : public CollisionDirection {
public:
  [[nodiscard]] auto
  distanceSubjectPenetratesObject(Rectangle subjectRectangle,
                                  Rectangle objectRectangle) const
      -> int override {
    return distanceFirstExceedsSecondHorizontally(objectRectangle,
                                                  subjectRectangle);
  }
};

static auto passesThrough(Rectangle subjectRectangle, Rectangle objectRectangle,
                          Velocity subjectVelocity,
                          const CollisionDirection &direction,
                          const CollisionAxis &axis) -> bool {
  const auto subjectDoesNotExceedObjectsUpperBoundary =
      isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
          objectRectangle, subjectRectangle));
  const auto subjectDoesNotPrecedeObjectsLowerBoundary =
      isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
          subjectRectangle, objectRectangle));
  const auto subjectPassesThroughObjectTowardUpperBoundary{
      axis.headingTowardUpperBoundary(subjectVelocity) &&
      isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
          axis.applyVelocityParallelToSurface(subjectRectangle,
                                              subjectVelocity),
          objectRectangle)) &&
      subjectDoesNotExceedObjectsUpperBoundary &&
      axis.surfaceRelativeSlope(subjectVelocity) >
          RationalNumber{-(direction.distanceSubjectPenetratesObject(
                               subjectRectangle, objectRectangle) +
                           1),
                         axis.distanceFirstExceedsSecondParallelToSurface(
                             objectRectangle, subjectRectangle) +
                             1}};
  const auto subjectPassesThroughObjectTowardLowerBoundary{
      axis.headingTowardLowerBoundary(subjectVelocity) &&
      isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
          objectRectangle, axis.applyVelocityParallelToSurface(
                               subjectRectangle, subjectVelocity))) &&
      subjectDoesNotPrecedeObjectsLowerBoundary &&
      axis.surfaceRelativeSlope(subjectVelocity) <
          RationalNumber{direction.distanceSubjectPenetratesObject(
                             subjectRectangle, objectRectangle) +
                             1,
                         axis.distanceFirstExceedsSecondParallelToSurface(
                             subjectRectangle, objectRectangle) +
                             1}};
  const auto subjectPassesThroughObjectDirectly{
      !axis.headingTowardUpperBoundary(subjectVelocity) &&
      !axis.headingTowardLowerBoundary(subjectVelocity) &&
      subjectDoesNotPrecedeObjectsLowerBoundary &&
      subjectDoesNotExceedObjectsUpperBoundary};
  return subjectPassesThroughObjectTowardLowerBoundary ||
         subjectPassesThroughObjectTowardUpperBoundary ||
         subjectPassesThroughObjectDirectly;
}

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
  Window(int width, int height)
      : window{SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED, width, height,
                                SDL_WINDOW_SHOWN)} {
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

static void handleVerticalCollisions(Rectangle &playerRectangle,
                                     Velocity &playerVelocity,
                                     JumpState &playerJumpState,
                                     const Rectangle &blockRectangle,
                                     const Rectangle &floorRectangle) {
  const auto playerIsAboveTopOfBlock =
      distanceFirstExceedsSecondVertically(playerRectangle, blockRectangle) < 0;
  const auto playerIsBelowBottomOfBlock =
      distanceFirstExceedsSecondVertically(blockRectangle, playerRectangle) < 0;
  const auto playerWillBeBelowTopOfBlock =
      isNonnegative(distanceFirstExceedsSecondVertically(
          applyVerticalVelocity(playerRectangle, playerVelocity),
          blockRectangle));
  const auto playerWillBeAboveBottomOfBlock =
      isNonnegative(distanceFirstExceedsSecondVertically(
          blockRectangle,
          applyVerticalVelocity(playerRectangle, playerVelocity)));
  if (playerIsAboveTopOfBlock && playerWillBeBelowTopOfBlock &&
      passesThrough(playerRectangle, blockRectangle, playerVelocity,
                    CollisionFromBelow{}, VerticalCollision{}))
    onPlayerHitGround(playerVelocity.vertical, playerRectangle, playerJumpState,
                      topEdge(blockRectangle));
  else if (playerIsBelowBottomOfBlock && playerWillBeAboveBottomOfBlock &&
           passesThrough(playerRectangle, blockRectangle, playerVelocity,
                         CollisionFromAbove{}, VerticalCollision{})) {
    playerVelocity.vertical = {0, 1};
    playerRectangle.origin.y = bottomEdge(blockRectangle) + 1;
  } else if (distanceFirstExceedsSecondVertically(
                 applyVerticalVelocity(playerRectangle, playerVelocity),
                 floorRectangle) >= 0)
    onPlayerHitGround(playerVelocity.vertical, playerRectangle, playerJumpState,
                      topEdge(floorRectangle));
}

static void handleHorizontalCollisions(Rectangle &playerRectangle,
                                       Velocity &playerVelocity,
                                       const Rectangle &blockRectangle) {
  const auto playerIsBeforeLeftOfBlock =
      distanceFirstExceedsSecondHorizontally(playerRectangle, blockRectangle) <
      0;
  const auto playerIsAfterRightOfBlock =
      distanceFirstExceedsSecondHorizontally(blockRectangle, playerRectangle) <
      0;
  const auto playerWillBeAheadOfLeftOfBlock =
      isNonnegative(distanceFirstExceedsSecondHorizontally(
          applyHorizontalVelocity(playerRectangle, playerVelocity),
          blockRectangle));
  const auto playerWillBeBeforeRightOfBlock =
      isNonnegative(distanceFirstExceedsSecondHorizontally(
          blockRectangle,
          applyHorizontalVelocity(playerRectangle, playerVelocity)));
  if (playerIsBeforeLeftOfBlock && playerWillBeAheadOfLeftOfBlock &&
      passesThrough(playerRectangle, blockRectangle, playerVelocity,
                    CollisionFromRight{}, HorizontalCollision{})) {
    playerVelocity.horizontal = 0;
    playerRectangle.origin.x = leftEdge(blockRectangle) - playerRectangle.width;
  } else if (playerIsAfterRightOfBlock && playerWillBeBeforeRightOfBlock &&
             passesThrough(playerRectangle, blockRectangle, playerVelocity,
                           CollisionFromLeft{}, HorizontalCollision{})) {
    playerVelocity.horizontal = 0;
    playerRectangle.origin.x = rightEdge(blockRectangle) + 1;
  }
}

static void shiftBackground(Rectangle &backgroundSourceRectangle,
                            int backgroundSourceWidth,
                            const Rectangle &playerRectangle, int cameraWidth) {
  const auto playerDistanceRightOfCameraCenter{
      leftEdge(playerRectangle) + playerRectangle.width / 2 - cameraWidth / 2 -
      leftEdge(backgroundSourceRectangle)};
  const auto distanceFromBackgroundRightEdgeToEnd{
      backgroundSourceWidth - rightEdge(backgroundSourceRectangle) - 1};
  if (playerDistanceRightOfCameraCenter > 0 &&
      distanceFromBackgroundRightEdgeToEnd > 0)
    backgroundSourceRectangle.origin.x +=
        std::min(distanceFromBackgroundRightEdgeToEnd,
                 playerDistanceRightOfCameraCenter);
  else if (playerDistanceRightOfCameraCenter < 0 &&
           leftEdge(backgroundSourceRectangle) > 0)
    backgroundSourceRectangle.origin.x +=
        std::max(-leftEdge(backgroundSourceRectangle),
                 playerDistanceRightOfCameraCenter);
}

static void applyHorizontalForces(Velocity &playerVelocity, int groundFriction,
                                  int playerMaxHorizontalSpeed,
                                  int playerRunAcceleration) {
  const auto *keyStates{SDL_GetKeyboardState(nullptr)};
  if (pressing(keyStates, SDL_SCANCODE_LEFT))
    playerVelocity.horizontal -= playerRunAcceleration;
  if (pressing(keyStates, SDL_SCANCODE_RIGHT))
    playerVelocity.horizontal += playerRunAcceleration;
  playerVelocity.horizontal =
      withFriction(clamp(playerVelocity.horizontal, playerMaxHorizontalSpeed),
                   groundFriction);
}

static void applyVerticalForces(Velocity &playerVelocity,
                                JumpState &playerJumpState,
                                int playerJumpAcceleration,
                                RationalNumber gravity) {
  const auto *keyStates{SDL_GetKeyboardState(nullptr)};
  if (pressing(keyStates, SDL_SCANCODE_UP) &&
      playerJumpState == JumpState::grounded) {
    playerJumpState = JumpState::started;
    playerVelocity.vertical += playerJumpAcceleration;
  }
  playerVelocity.vertical += gravity;
  if (!pressing(keyStates, SDL_SCANCODE_UP) &&
      playerJumpState == JumpState::started) {
    playerJumpState = JumpState::released;
    if (playerVelocity.vertical < 0)
      playerVelocity.vertical = {0, 1};
  }
}

static void present(sdl_wrappers::Renderer &rendererWrapper,
                    sdl_wrappers::Texture &playerTextureWrapper,
                    sdl_wrappers::Texture &backgroundTextureWrapper,
                    const SDL_Rect &playerSourceRect,
                    const Rectangle &playerRectangle,
                    const Rectangle &backgroundSourceRectangle, int cameraWidth,
                    int cameraHeight, int pixelScale) {
  const auto backgroundSourceRectConverted{
      toSDLRect(backgroundSourceRectangle)};
  const Rectangle backgroundRectangle{{0, 0}, cameraWidth, cameraHeight};
  const auto backgroundProjection{toSDLRect(backgroundRectangle * pixelScale)};
  SDL_RenderCopyEx(rendererWrapper.renderer, backgroundTextureWrapper.texture,
                   &backgroundSourceRectConverted, &backgroundProjection, 0,
                   nullptr, SDL_FLIP_NONE);
  auto playerPreProjection{playerRectangle};
  playerPreProjection.origin.x -= leftEdge(backgroundSourceRectangle);
  const auto playerProjection{toSDLRect(playerPreProjection * pixelScale)};
  SDL_RenderCopyEx(rendererWrapper.renderer, playerTextureWrapper.texture,
                   &playerSourceRect, &playerProjection, 0, nullptr,
                   SDL_FLIP_NONE);
  SDL_RenderPresent(rendererWrapper.renderer);
}

static void flushEvents(bool &playing) {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0)
    playing = event.type != SDL_QUIT;
}

static auto run(const std::string &playerImagePath,
                const std::string &backgroundImagePath) -> int {
  sdl_wrappers::Init scopedInitialization;
  constexpr auto pixelScale{4};
  const auto cameraWidth{256};
  const auto cameraHeight{240};
  constexpr auto screenWidth{cameraWidth * pixelScale};
  constexpr auto screenHeight{cameraHeight * pixelScale};
  sdl_wrappers::Window windowWrapper{screenWidth, screenHeight};
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
  sdl_wrappers::Texture playerTextureWrapper{rendererWrapper.renderer,
                                             playerImageSurfaceWrapper.surface};
  sdl_wrappers::Texture backgroundTextureWrapper{
      rendererWrapper.renderer, backgroundImageSurfaceWrapper.surface};
  const Rectangle floorRectangle{Point{0, cameraHeight - 32},
                                 backgroundSourceWidth, 32};
  const RationalNumber gravity{1, 4};
  const auto groundFriction{1};
  const auto playerMaxHorizontalSpeed{4};
  const auto playerJumpAcceleration{-6};
  const auto playerRunAcceleration{2};
  Rectangle playerRectangle{Point{0, topEdge(floorRectangle) - playerHeight},
                            playerWidth, playerHeight};
  Velocity playerVelocity{{0, 1}, 0};
  auto playerJumpState{JumpState::grounded};
  const Rectangle blockRectangle{Point{256, 144}, 15, 15};
  Rectangle backgroundSourceRectangle{Point{0, 0}, cameraWidth, cameraHeight};
  auto playing{true};
  while (playing) {
    flushEvents(playing);
    applyHorizontalForces(playerVelocity, groundFriction,
                          playerMaxHorizontalSpeed, playerRunAcceleration);
    applyVerticalForces(playerVelocity, playerJumpState, playerJumpAcceleration,
                        gravity);
    handleVerticalCollisions(playerRectangle, playerVelocity, playerJumpState,
                             blockRectangle, floorRectangle);
    handleHorizontalCollisions(playerRectangle, playerVelocity, blockRectangle);
    playerRectangle = applyVerticalVelocity(
        applyHorizontalVelocity(playerRectangle, playerVelocity),
        playerVelocity);
    shiftBackground(backgroundSourceRectangle, backgroundSourceWidth,
                    playerRectangle, cameraWidth);
    present(rendererWrapper, playerTextureWrapper, backgroundTextureWrapper,
            playerSourceRect, playerRectangle, backgroundSourceRectangle,
            cameraWidth, cameraHeight, pixelScale);
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
