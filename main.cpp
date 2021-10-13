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
#include <vector>

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

struct PlayerState {
  Rectangle rectangle;
  Velocity velocity;
  JumpState jumpState;
};

constexpr auto operator-(Velocity a) -> Velocity {
  return {-a.vertical, -a.horizontal};
}

constexpr auto shiftHorizontally(Rectangle a, int b) -> Rectangle {
  a.origin.x += b;
  return a;
}

constexpr auto applyHorizontalVelocity(Rectangle a, Velocity b) -> Rectangle {
  return shiftHorizontally(a, b.horizontal);
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

constexpr auto isNegative(int a) -> bool { return a < 0; }

constexpr auto isNonnegative(int a) -> bool { return !isNegative(a); }

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

static auto subjectPassesThroughObjectTowardUpperBoundary(
    Rectangle &subjectRectangle, Rectangle &objectRectangle,
    Velocity &subjectVelocity, const CollisionDirection &direction,
    const CollisionAxis &axis) -> bool {
  return axis.headingTowardUpperBoundary(subjectVelocity) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             axis.applyVelocityParallelToSurface(subjectRectangle,
                                                 subjectVelocity),
             objectRectangle)) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             objectRectangle, subjectRectangle)) &&
         axis.surfaceRelativeSlope(subjectVelocity) >
             RationalNumber{-(direction.distanceSubjectPenetratesObject(
                                  subjectRectangle, objectRectangle) +
                              1),
                            axis.distanceFirstExceedsSecondParallelToSurface(
                                objectRectangle, subjectRectangle) +
                                1};
}

static auto subjectPassesThroughObjectTowardLowerBoundary(
    Rectangle &subjectRectangle, Rectangle &objectRectangle,
    Velocity &subjectVelocity, const CollisionDirection &direction,
    const CollisionAxis &axis) -> bool {
  return axis.headingTowardLowerBoundary(subjectVelocity) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             objectRectangle, axis.applyVelocityParallelToSurface(
                                  subjectRectangle, subjectVelocity))) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             subjectRectangle, objectRectangle)) &&
         axis.surfaceRelativeSlope(subjectVelocity) <
             RationalNumber{direction.distanceSubjectPenetratesObject(
                                subjectRectangle, objectRectangle) +
                                1,
                            axis.distanceFirstExceedsSecondParallelToSurface(
                                subjectRectangle, objectRectangle) +
                                1};
}

static auto passesThrough(Rectangle subjectRectangle, Rectangle objectRectangle,
                          Velocity subjectVelocity,
                          const CollisionDirection &direction,
                          const CollisionAxis &axis) -> bool {
  if (isNonnegative(direction.distanceSubjectPenetratesObject(
          subjectRectangle, objectRectangle)) ||
      direction.distanceSubjectPenetratesObject(
          axis.applyVelocityNormalToSurface(subjectRectangle, subjectVelocity),
          objectRectangle) < 0)
    return false;
  if (isNegative(axis.distanceFirstExceedsSecondParallelToSurface(
          subjectRectangle, objectRectangle)) ||
      axis.headingTowardUpperBoundary(subjectVelocity))
    return subjectPassesThroughObjectTowardUpperBoundary(
        subjectRectangle, objectRectangle, subjectVelocity, direction, axis);
  if (isNegative(axis.distanceFirstExceedsSecondParallelToSurface(
          objectRectangle, subjectRectangle)) ||
      axis.headingTowardLowerBoundary(subjectVelocity))
    return subjectPassesThroughObjectTowardLowerBoundary(
        subjectRectangle, objectRectangle, subjectVelocity, direction, axis);
  return true;
}

static auto onPlayerHitGround(PlayerState playerState, int ground)
    -> PlayerState {
  playerState.velocity.vertical = {0, 1};
  playerState.rectangle.origin.y = ground - playerState.rectangle.height;
  playerState.jumpState = JumpState::grounded;
  return playerState;
}

static auto handleVerticalCollisions(PlayerState playerState,
                                     const Rectangle &blockRectangle,
                                     const Rectangle &floorRectangle)
    -> PlayerState {
  if (passesThrough(playerState.rectangle, blockRectangle, playerState.velocity,
                    CollisionFromBelow{}, VerticalCollision{}))
    return onPlayerHitGround(playerState, topEdge(blockRectangle));
  if (passesThrough(playerState.rectangle, blockRectangle, playerState.velocity,
                    CollisionFromAbove{}, VerticalCollision{})) {
    playerState.velocity.vertical = {0, 1};
    playerState.rectangle.origin.y = bottomEdge(blockRectangle) + 1;
    return playerState;
  }
  if (distanceFirstExceedsSecondVertically(
          applyVerticalVelocity(playerState.rectangle, playerState.velocity),
          floorRectangle) >= 0)
    return onPlayerHitGround(playerState, topEdge(floorRectangle));
  return playerState;
}

static auto sortByLeftEdge(std::vector<Rectangle> objects)
    -> std::vector<Rectangle> {
  std::sort(objects.begin(), objects.end(),
            [](Rectangle a, Rectangle b) { return leftEdge(a) < leftEdge(b); });
  return objects;
}

static auto sortByRightEdge(std::vector<Rectangle> objects)
    -> std::vector<Rectangle> {
  std::sort(objects.begin(), objects.end(), [](Rectangle a, Rectangle b) {
    return rightEdge(a) > rightEdge(b);
  });
  return objects;
}

static auto handleHorizontalCollisions(PlayerState playerState,
                                       const std::vector<Rectangle> &objects,
                                       const Rectangle &levelRectangle)
    -> PlayerState {
  for (const auto object : sortByLeftEdge(objects))
    if (passesThrough(playerState.rectangle, object, playerState.velocity,
                      CollisionFromRight{}, HorizontalCollision{})) {
      playerState.velocity.horizontal = 0;
      playerState.rectangle.origin.x =
          leftEdge(object) - playerState.rectangle.width;
      return playerState;
    }
  if (rightEdge(applyHorizontalVelocity(playerState.rectangle,
                                        playerState.velocity)) -
          rightEdge(levelRectangle) >=
      0) {
    playerState.velocity.horizontal = 0;
    playerState.rectangle.origin.x =
        rightEdge(levelRectangle) - playerState.rectangle.width;
    return playerState;
  }

  for (const auto object : sortByRightEdge(objects))
    if (passesThrough(playerState.rectangle, object, playerState.velocity,
                      CollisionFromLeft{}, HorizontalCollision{})) {
      playerState.velocity.horizontal = 0;
      playerState.rectangle.origin.x = rightEdge(object) + 1;
      return playerState;
    }
  if (leftEdge(levelRectangle) -
          leftEdge(applyHorizontalVelocity(playerState.rectangle,
                                           playerState.velocity)) >=
      0) {
    playerState.velocity.horizontal = 0;
    playerState.rectangle.origin.x = leftEdge(levelRectangle) + 1;
    return playerState;
  }
  return playerState;
}

static auto shiftBackground(Rectangle backgroundSourceRectangle,
                            int backgroundSourceWidth,
                            const Rectangle &playerRectangle, int cameraWidth)
    -> Rectangle {
  const auto playerDistanceRightOfCameraCenter{
      leftEdge(playerRectangle) + playerRectangle.width / 2 - cameraWidth / 2 -
      leftEdge(backgroundSourceRectangle)};
  const auto distanceFromBackgroundRightEdgeToEnd{
      backgroundSourceWidth - rightEdge(backgroundSourceRectangle) - 1};
  if (playerDistanceRightOfCameraCenter > 0 &&
      distanceFromBackgroundRightEdgeToEnd > 0)
    return shiftHorizontally(backgroundSourceRectangle,
                             std::min(distanceFromBackgroundRightEdgeToEnd,
                                      playerDistanceRightOfCameraCenter));
  if (playerDistanceRightOfCameraCenter < 0 &&
      leftEdge(backgroundSourceRectangle) > 0)
    return shiftHorizontally(backgroundSourceRectangle,
                             std::max(-leftEdge(backgroundSourceRectangle),
                                      playerDistanceRightOfCameraCenter));
  return backgroundSourceRectangle;
}

static auto applyVelocity(PlayerState playerState) -> PlayerState {
  playerState.rectangle = applyVerticalVelocity(
      applyHorizontalVelocity(playerState.rectangle, playerState.velocity),
      playerState.velocity);
  return playerState;
}

static auto moveTowardPlayer(Rectangle subject, int horizontalVelocity,
                             const PlayerState &playerState) -> Rectangle {
  if (leftEdge(playerState.rectangle) < leftEdge(subject))
    return shiftHorizontally(subject, -horizontalVelocity);
  if (leftEdge(playerState.rectangle) > leftEdge(subject))
    return shiftHorizontally(subject, horizontalVelocity);
  return subject;
}
} // namespace sbash64::game

namespace sbash64::game {
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

static auto applyHorizontalForces(PlayerState playerState, int groundFriction,
                                  int playerMaxHorizontalSpeed,
                                  int playerRunAcceleration) -> PlayerState {
  const auto *keyStates{SDL_GetKeyboardState(nullptr)};
  if (pressing(keyStates, SDL_SCANCODE_LEFT))
    playerState.velocity.horizontal -= playerRunAcceleration;
  if (pressing(keyStates, SDL_SCANCODE_RIGHT))
    playerState.velocity.horizontal += playerRunAcceleration;
  playerState.velocity.horizontal = withFriction(
      clamp(playerState.velocity.horizontal, playerMaxHorizontalSpeed),
      groundFriction);
  return playerState;
}

static auto applyVerticalForces(PlayerState playerState,
                                int playerJumpAcceleration,
                                RationalNumber gravity) -> PlayerState {
  const auto *keyStates{SDL_GetKeyboardState(nullptr)};
  if (pressing(keyStates, SDL_SCANCODE_UP) &&
      playerState.jumpState == JumpState::grounded) {
    playerState.jumpState = JumpState::started;
    playerState.velocity.vertical += playerJumpAcceleration;
  }
  playerState.velocity.vertical += gravity;
  if (!pressing(keyStates, SDL_SCANCODE_UP) &&
      playerState.jumpState == JumpState::started) {
    playerState.jumpState = JumpState::released;
    if (playerState.velocity.vertical < 0)
      playerState.velocity.vertical = {0, 1};
  }
  return playerState;
}

static void present(const sdl_wrappers::Renderer &rendererWrapper,
                    const sdl_wrappers::Texture &textureWrapper,
                    const Rectangle &sourceRectangle, int pixelScale,
                    const Rectangle &destinationRectangle) {
  const auto projection{toSDLRect(destinationRectangle * pixelScale)};
  const auto sourceSDLRect{toSDLRect(sourceRectangle)};
  SDL_RenderCopyEx(rendererWrapper.renderer, textureWrapper.texture,
                   &sourceSDLRect, &projection, 0, nullptr, SDL_FLIP_NONE);
}

static void flushEvents(bool &playing) {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0)
    playing = event.type != SDL_QUIT;
}

static auto run(const std::string &playerImagePath,
                const std::string &backgroundImagePath,
                const std::string &enemyImagePath) -> int {
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
  const Rectangle playerSourceRect{Point{1, 9}, playerWidth, playerHeight};
  sdl_wrappers::ImageSurface backgroundImageSurfaceWrapper{backgroundImagePath};
  const auto backgroundSourceWidth{backgroundImageSurfaceWrapper.surface->w};
  sdl_wrappers::ImageSurface enemyImageSurfaceWrapper{enemyImagePath};
  SDL_SetColorKey(enemyImageSurfaceWrapper.surface, SDL_TRUE,
                  getpixel(enemyImageSurfaceWrapper.surface, 1, 28));
  const auto enemyWidth{16};
  const auto enemyHeight{16};
  const Rectangle enemySourceRect{Point{1, 28}, enemyWidth, enemyHeight};
  sdl_wrappers::Texture playerTextureWrapper{rendererWrapper.renderer,
                                             playerImageSurfaceWrapper.surface};
  sdl_wrappers::Texture backgroundTextureWrapper{
      rendererWrapper.renderer, backgroundImageSurfaceWrapper.surface};
  sdl_wrappers::Texture enemyTextureWrapper{rendererWrapper.renderer,
                                            enemyImageSurfaceWrapper.surface};
  const Rectangle floorRectangle{Point{0, cameraHeight - 32},
                                 backgroundSourceWidth, 32};
  const Rectangle levelRectangle{Point{-1, -1}, backgroundSourceWidth + 1,
                                 cameraHeight + 1};
  const RationalNumber gravity{1, 4};
  const auto groundFriction{1};
  const auto playerMaxHorizontalSpeed{4};
  const auto playerJumpAcceleration{-6};
  const auto playerRunAcceleration{2};
  PlayerState playerState{
      Rectangle{Point{0, topEdge(floorRectangle) - playerHeight}, playerWidth,
                playerHeight},
      Velocity{{0, 1}, 0}, JumpState::grounded};
  const auto enemyHorizontalVelocity{1};
  Rectangle enemyRectangle{Point{140, topEdge(floorRectangle) - enemyHeight},
                           enemyWidth, enemyHeight};
  const Rectangle blockRectangle{Point{256, 144}, 15, 15};
  Rectangle backgroundSourceRectangle{Point{0, 0}, cameraWidth, cameraHeight};
  auto playing{true};
  while (playing) {
    flushEvents(playing);
    playerState = applyVelocity(handleHorizontalCollisions(
        handleVerticalCollisions(
            applyVerticalForces(applyHorizontalForces(playerState,
                                                      groundFriction,
                                                      playerMaxHorizontalSpeed,
                                                      playerRunAcceleration),
                                playerJumpAcceleration, gravity),
            blockRectangle, floorRectangle),
        {blockRectangle}, levelRectangle));
    enemyRectangle =
        moveTowardPlayer(enemyRectangle, enemyHorizontalVelocity, playerState);
    backgroundSourceRectangle =
        shiftBackground(backgroundSourceRectangle, backgroundSourceWidth,
                        playerState.rectangle, cameraWidth);
    present(rendererWrapper, backgroundTextureWrapper,
            backgroundSourceRectangle, pixelScale,
            {Point{0, 0}, cameraWidth, cameraHeight});
    present(rendererWrapper, enemyTextureWrapper, enemySourceRect, pixelScale,
            shiftHorizontally(enemyRectangle,
                              -leftEdge(backgroundSourceRectangle)));
    present(rendererWrapper, playerTextureWrapper, playerSourceRect, pixelScale,
            shiftHorizontally(playerState.rectangle,
                              -leftEdge(backgroundSourceRectangle)));
    SDL_RenderPresent(rendererWrapper.renderer);
  }
  return EXIT_SUCCESS;
}
} // namespace sbash64::game

int main(int argc, char *argv[]) {
  if (argc < 4)
    return EXIT_FAILURE;
  try {
    return sbash64::game::run(argv[1], argv[2], argv[3]);
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
