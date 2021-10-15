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
using distance_type = int;

constexpr auto isNegative(distance_type a) -> bool { return a < 0; }

constexpr auto isNonnegative(distance_type a) -> bool { return !isNegative(a); }

struct RationalDistance {
  distance_type numerator;
  distance_type denominator;

  auto operator==(const RationalDistance &) const -> bool = default;
};

constexpr auto operator+=(RationalDistance &a, RationalDistance b)
    -> RationalDistance & {
  const auto smallerDenominator{std::min(a.denominator, b.denominator)};
  const auto largerDenominator{std::max(a.denominator, b.denominator)};
  auto commonDenominator{smallerDenominator};
  auto candidateDenominator{largerDenominator};
  while (true) {
    while (
        commonDenominator <
        std::min(std::numeric_limits<distance_type>::max() - smallerDenominator,
                 candidateDenominator))
      commonDenominator += smallerDenominator;
    if (commonDenominator != candidateDenominator) {
      if (candidateDenominator <
          std::numeric_limits<distance_type>::max() - largerDenominator)
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

constexpr auto operator+=(RationalDistance &a, distance_type b)
    -> RationalDistance {
  a.numerator += a.denominator * b;
  return a;
}

constexpr auto operator+(RationalDistance a, distance_type b)
    -> RationalDistance {
  return a += b;
}

constexpr auto operator+(RationalDistance a, RationalDistance b)
    -> RationalDistance {
  return a += b;
}

constexpr auto operator-(RationalDistance a) -> RationalDistance {
  return {-a.numerator, a.denominator};
}

constexpr auto operator/(RationalDistance a, RationalDistance b)
    -> RationalDistance {
  return {a.numerator * b.denominator, a.denominator * b.numerator};
}

constexpr auto operator/(RationalDistance a, distance_type b)
    -> RationalDistance {
  return {a.numerator, a.denominator * b};
}

constexpr auto operator<(RationalDistance a, RationalDistance b) -> bool {
  return (isNegative(a.denominator) ^ isNegative(b.denominator)) != 0
             ? a.numerator * b.denominator > b.numerator * a.denominator
             : a.numerator * b.denominator < b.numerator * a.denominator;
}

constexpr auto operator>(RationalDistance a, RationalDistance b) -> bool {
  return b < a;
}

constexpr auto operator<=(RationalDistance a, RationalDistance b) -> bool {
  return !(a > b);
}

constexpr auto operator>=(RationalDistance a, RationalDistance b) -> bool {
  return !(a < b);
}

constexpr auto operator<(RationalDistance a, distance_type b) -> bool {
  return isNegative(a.denominator) ? a.numerator > b * a.denominator
                                   : a.numerator < b * a.denominator;
}

constexpr auto operator>(RationalDistance a, distance_type b) -> bool {
  return isNegative(a.denominator) ? a.numerator < b * a.denominator
                                   : a.numerator > b * a.denominator;
}

constexpr auto absoluteValue(distance_type a) -> distance_type {
  return isNegative(a) ? -a : a;
}

constexpr auto round(RationalDistance a) -> distance_type {
  const auto division{a.numerator / a.denominator};
  if (absoluteValue(a.numerator) % a.denominator <
      (absoluteValue(a.denominator) + 1) / 2)
    return division;
  return (isNegative(a.numerator) ^ isNegative(a.denominator)) != 0
             ? division - 1
             : division + 1;
}

static_assert(7 % 4 == 3, "do I understand cpp modulus?");
static_assert(7 % -4 == 3, "do I understand cpp modulus?");
static_assert(-7 % 4 == -3, "do I understand cpp modulus?");
static_assert(-7 % -4 == -3, "do I understand cpp modulus?");

static_assert(RationalDistance{3, 4} + RationalDistance{5, 6} ==
                  RationalDistance{19, 12},
              "rational number arithmetic error");

static_assert(RationalDistance{4, 7} + RationalDistance{2, 3} ==
                  RationalDistance{26, 21},
              "rational number arithmetic error");

static_assert(RationalDistance{4, 7} + RationalDistance{-2, 3} ==
                  RationalDistance{-2, 21},
              "rational number arithmetic error");

static_assert(RationalDistance{4, 7} + 3 == RationalDistance{25, 7},
              "rational number arithmetic error");

static_assert(RationalDistance{4, 7} / RationalDistance{2, 3} ==
                  RationalDistance{12, 14},
              "rational number arithmetic error");

static_assert(RationalDistance{4, 7} / 2 == RationalDistance{4, 14},
              "rational number arithmetic error");

static_assert(RationalDistance{19, 12} < RationalDistance{7, 3},
              "rational number comparison error");

static_assert(RationalDistance{-1, 2} < RationalDistance{1, 3},
              "rational number comparison error");

static_assert(RationalDistance{-1, 2} < RationalDistance{1, -3},
              "rational number comparison error");

static_assert(RationalDistance{1, -2} < RationalDistance{-1, 3},
              "rational number comparison error");

static_assert(RationalDistance{1, -2} < RationalDistance{-1, -3},
              "rational number comparison error");

static_assert(RationalDistance{-1, -2} > RationalDistance{-1, -3},
              "rational number comparison error");

static_assert(RationalDistance{2, 3} > RationalDistance{1, 4},
              "rational number comparison error");

static_assert(RationalDistance{-2, 3} > -1, "rational number comparison error");

static_assert(RationalDistance{-2, 3} < 0, "rational number comparison error");

static_assert(RationalDistance{2, -3} < 0, "rational number comparison error");

static_assert(RationalDistance{-2, -3} > 0, "rational number comparison error");

static_assert(RationalDistance{2, 3} > RationalDistance{1, 4},
              "rational number comparison error");

static_assert(round(RationalDistance{19, 12}) == 2,
              "rational number round error");

static_assert(round(RationalDistance{3, 7}) == 0,
              "rational number round error");

static_assert(round(RationalDistance{-3, 7}) == 0,
              "rational number round error");

static_assert(round(RationalDistance{-4, 7}) == -1,
              "rational number round error");

enum class JumpState { grounded, started, released };

struct Point {
  distance_type x;
  distance_type y;
};

struct Rectangle {
  Point origin;
  distance_type width;
  distance_type height;
};

struct Velocity {
  RationalDistance vertical;
  distance_type horizontal;
};

struct MovingObject {
  Rectangle rectangle;
  Velocity velocity;
};

struct PlayerState {
  MovingObject object;
  JumpState jumpState;
};

constexpr auto operator-(Velocity a) -> Velocity {
  return {-a.vertical, -a.horizontal};
}

constexpr auto shiftHorizontally(Rectangle a, distance_type b) -> Rectangle {
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

constexpr auto topEdge(Rectangle a) -> distance_type { return a.origin.y; }

constexpr auto leftEdge(Rectangle a) -> distance_type { return a.origin.x; }

constexpr auto rightEdge(Rectangle a) -> distance_type {
  return a.origin.x + a.width - 1;
}

constexpr auto bottomEdge(Rectangle a) -> distance_type {
  return a.origin.y + a.height - 1;
}

constexpr auto operator*=(Rectangle &a, distance_type scale) -> Rectangle & {
  a.origin.x *= scale;
  a.origin.y *= scale;
  a.width *= scale;
  a.height *= scale;
  return a;
}

constexpr auto operator*(Rectangle a, distance_type scale) -> Rectangle {
  return a *= scale;
}

constexpr auto distanceFirstExceedsSecondVertically(Rectangle a, Rectangle b)
    -> distance_type {
  return bottomEdge(a) - topEdge(b);
}

constexpr auto distanceFirstExceedsSecondHorizontally(Rectangle a, Rectangle b)
    -> distance_type {
  return rightEdge(a) - leftEdge(b);
}

constexpr auto clamp(distance_type velocity, distance_type limit)
    -> distance_type {
  return std::clamp(velocity, -limit, limit);
}

constexpr auto withFriction(distance_type velocity, distance_type friction)
    -> distance_type {
  return (isNegative(velocity) ? -1 : 1) *
         std::max(0, std::abs(velocity) - friction);
}

class CollisionDirection {
public:
  [[nodiscard]] virtual auto distanceSubjectPenetratesObject(Rectangle a,
                                                             Rectangle b) const
      -> distance_type = 0;
};

class CollisionAxis {
public:
  [[nodiscard]] virtual auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> distance_type = 0;
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
      -> RationalDistance = 0;
};

class HorizontalCollision : public CollisionAxis {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> distance_type override {
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
    return isNegative(round(a.vertical));
  }

  [[nodiscard]] auto surfaceRelativeSlope(Velocity a) const
      -> RationalDistance override {
    return RationalDistance{std::abs(a.horizontal), round(a.vertical)};
  }
};

class VerticalCollision : public CollisionAxis {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> distance_type override {
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
    return isNegative(a.horizontal);
  }

  [[nodiscard]] auto surfaceRelativeSlope(Velocity a) const
      -> RationalDistance override {
    return RationalDistance{std::abs(round(a.vertical)), a.horizontal};
  }
};

class CollisionFromBelow : public CollisionDirection {
public:
  [[nodiscard]] auto
  distanceSubjectPenetratesObject(Rectangle subjectRectangle,
                                  Rectangle objectRectangle) const
      -> distance_type override {
    return distanceFirstExceedsSecondVertically(subjectRectangle,
                                                objectRectangle);
  }
};

class CollisionFromAbove : public CollisionDirection {
public:
  [[nodiscard]] auto
  distanceSubjectPenetratesObject(Rectangle subjectRectangle,
                                  Rectangle objectRectangle) const
      -> distance_type override {
    return distanceFirstExceedsSecondVertically(objectRectangle,
                                                subjectRectangle);
  }
};

class CollisionFromRight : public CollisionDirection {
public:
  [[nodiscard]] auto
  distanceSubjectPenetratesObject(Rectangle subjectRectangle,
                                  Rectangle objectRectangle) const
      -> distance_type override {
    return distanceFirstExceedsSecondHorizontally(subjectRectangle,
                                                  objectRectangle);
  }
};

class CollisionFromLeft : public CollisionDirection {
public:
  [[nodiscard]] auto
  distanceSubjectPenetratesObject(Rectangle subjectRectangle,
                                  Rectangle objectRectangle) const
      -> distance_type override {
    return distanceFirstExceedsSecondHorizontally(objectRectangle,
                                                  subjectRectangle);
  }
};

static auto passesThroughTowardUpperBoundary(
    MovingObject movingObject, Rectangle stationaryObjectRectangle,
    const CollisionDirection &direction, const CollisionAxis &axis) -> bool {
  return axis.headingTowardUpperBoundary(movingObject.velocity) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             axis.applyVelocityParallelToSurface(movingObject.rectangle,
                                                 movingObject.velocity),
             stationaryObjectRectangle)) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             stationaryObjectRectangle, movingObject.rectangle)) &&
         axis.surfaceRelativeSlope(movingObject.velocity) >
             RationalDistance{
                 -(direction.distanceSubjectPenetratesObject(
                       movingObject.rectangle, stationaryObjectRectangle) +
                   1),
                 axis.distanceFirstExceedsSecondParallelToSurface(
                     stationaryObjectRectangle, movingObject.rectangle) +
                     1};
}

static auto passesThroughTowardLowerBoundary(
    MovingObject movingObject, Rectangle stationaryObjectRectangle,
    const CollisionDirection &direction, const CollisionAxis &axis) -> bool {
  return axis.headingTowardLowerBoundary(movingObject.velocity) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             stationaryObjectRectangle,
             axis.applyVelocityParallelToSurface(movingObject.rectangle,
                                                 movingObject.velocity))) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             movingObject.rectangle, stationaryObjectRectangle)) &&
         axis.surfaceRelativeSlope(movingObject.velocity) <
             RationalDistance{
                 direction.distanceSubjectPenetratesObject(
                     movingObject.rectangle, stationaryObjectRectangle) +
                     1,
                 axis.distanceFirstExceedsSecondParallelToSurface(
                     movingObject.rectangle, stationaryObjectRectangle) +
                     1};
}

static auto passesThrough(MovingObject movingObject,
                          Rectangle stationaryObjectRectangle,
                          const CollisionDirection &direction,
                          const CollisionAxis &axis) -> bool {
  if (isNonnegative(direction.distanceSubjectPenetratesObject(
          movingObject.rectangle, stationaryObjectRectangle)) ||
      isNegative(direction.distanceSubjectPenetratesObject(
          axis.applyVelocityNormalToSurface(movingObject.rectangle,
                                            movingObject.velocity),
          stationaryObjectRectangle)))
    return false;
  if (isNegative(axis.distanceFirstExceedsSecondParallelToSurface(
          movingObject.rectangle, stationaryObjectRectangle)) ||
      axis.headingTowardUpperBoundary(movingObject.velocity))
    return passesThroughTowardUpperBoundary(
        movingObject, stationaryObjectRectangle, direction, axis);
  if (isNegative(axis.distanceFirstExceedsSecondParallelToSurface(
          stationaryObjectRectangle, movingObject.rectangle)) ||
      axis.headingTowardLowerBoundary(movingObject.velocity))
    return passesThroughTowardLowerBoundary(
        movingObject, stationaryObjectRectangle, direction, axis);
  return true;
}

static auto collideVertically(MovingObject object, distance_type ground)
    -> MovingObject {
  object.velocity.vertical = {0, 1};
  object.rectangle.origin.y = ground - object.rectangle.height;
  return object;
}

static auto onPlayerHitGround(PlayerState playerState, distance_type ground)
    -> PlayerState {
  playerState.object = collideVertically(playerState.object, ground);
  playerState.jumpState = JumpState::grounded;
  return playerState;
}

static auto collideHorizontally(MovingObject object, distance_type leftEdge)
    -> MovingObject {
  object.velocity.horizontal = 0;
  object.rectangle.origin.x = leftEdge;
  return object;
}

static auto sortByTopEdge(std::vector<Rectangle> objects)
    -> std::vector<Rectangle> {
  std::sort(objects.begin(), objects.end(),
            [](Rectangle a, Rectangle b) { return topEdge(a) < topEdge(b); });
  return objects;
}

static auto sortByBottomEdge(std::vector<Rectangle> objects)
    -> std::vector<Rectangle> {
  std::sort(objects.begin(), objects.end(), [](Rectangle a, Rectangle b) {
    return bottomEdge(a) > bottomEdge(b);
  });
  return objects;
}

static auto handleVerticalCollisions(
    PlayerState playerState,
    const std::vector<Rectangle> &collisionFromBelowCandidates,
    const std::vector<Rectangle> &collisionFromAboveCandidates,
    const Rectangle &floorRectangle) -> PlayerState {
  for (const auto candidate : sortByTopEdge(collisionFromBelowCandidates))
    if (passesThrough(playerState.object, candidate, CollisionFromBelow{},
                      VerticalCollision{}))
      return onPlayerHitGround(playerState, topEdge(candidate));
  if (isNonnegative(distanceFirstExceedsSecondVertically(
          applyVerticalVelocity(playerState.object.rectangle,
                                playerState.object.velocity),
          floorRectangle)))
    return onPlayerHitGround(playerState, topEdge(floorRectangle));
  for (const auto object : sortByBottomEdge(collisionFromAboveCandidates))
    if (passesThrough(playerState.object, object, CollisionFromAbove{},
                      VerticalCollision{})) {
      playerState.object.velocity.vertical = {0, 1};
      playerState.object.rectangle.origin.y = bottomEdge(object) + 1;
      return playerState;
    }
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

static auto handleHorizontalCollisions(
    MovingObject object,
    const std::vector<Rectangle> &collisionFromRightCandidates,
    const std::vector<Rectangle> &collisionFromLeftCandidates,
    const Rectangle &levelRectangle) -> MovingObject {
  for (const auto candidate : sortByLeftEdge(collisionFromRightCandidates))
    if (passesThrough(object, candidate, CollisionFromRight{},
                      HorizontalCollision{}))
      return collideHorizontally(object,
                                 leftEdge(candidate) - object.rectangle.width);
  if (isNonnegative(rightEdge(applyHorizontalVelocity(object.rectangle,
                                                      object.velocity)) -
                    rightEdge(levelRectangle)))
    return collideHorizontally(object, rightEdge(levelRectangle) -
                                           object.rectangle.width);

  for (const auto candidate : sortByRightEdge(collisionFromLeftCandidates))
    if (passesThrough(object, candidate, CollisionFromLeft{},
                      HorizontalCollision{}))
      return collideHorizontally(object, rightEdge(candidate) + 1);
  if (isNonnegative(
          leftEdge(levelRectangle) -
          leftEdge(applyHorizontalVelocity(object.rectangle, object.velocity))))
    return collideHorizontally(object, leftEdge(levelRectangle) + 1);
  return object;
}

static auto shiftBackground(Rectangle backgroundSourceRectangle,
                            distance_type backgroundSourceWidth,
                            const Rectangle &playerRectangle,
                            distance_type cameraWidth) -> Rectangle {
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
  if (isNegative(playerDistanceRightOfCameraCenter) &&
      leftEdge(backgroundSourceRectangle) > 0)
    return shiftHorizontally(backgroundSourceRectangle,
                             std::max(-leftEdge(backgroundSourceRectangle),
                                      playerDistanceRightOfCameraCenter));
  return backgroundSourceRectangle;
}

static auto applyVelocity(MovingObject object) -> MovingObject {
  object.rectangle = applyVerticalVelocity(
      applyHorizontalVelocity(object.rectangle, object.velocity),
      object.velocity);
  return object;
}

static auto applyVelocity(PlayerState playerState) -> PlayerState {
  playerState.object = applyVelocity(playerState.object);
  return playerState;
}
} // namespace sbash64::game

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_image.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_scancode.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>

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

static auto applyHorizontalForces(PlayerState playerState,
                                  distance_type groundFriction,
                                  distance_type playerMaxHorizontalSpeed,
                                  distance_type playerRunAcceleration)
    -> PlayerState {
  const auto *keyStates{SDL_GetKeyboardState(nullptr)};
  if (pressing(keyStates, SDL_SCANCODE_LEFT))
    playerState.object.velocity.horizontal -= playerRunAcceleration;
  if (pressing(keyStates, SDL_SCANCODE_RIGHT))
    playerState.object.velocity.horizontal += playerRunAcceleration;
  playerState.object.velocity.horizontal = withFriction(
      clamp(playerState.object.velocity.horizontal, playerMaxHorizontalSpeed),
      groundFriction);
  return playerState;
}

static auto applyVerticalForces(PlayerState playerState,
                                distance_type playerJumpAcceleration,
                                RationalDistance gravity) -> PlayerState {
  const auto *keyStates{SDL_GetKeyboardState(nullptr)};
  if (pressing(keyStates, SDL_SCANCODE_UP) &&
      playerState.jumpState == JumpState::grounded) {
    playerState.jumpState = JumpState::started;
    playerState.object.velocity.vertical += playerJumpAcceleration;
  }
  playerState.object.velocity.vertical += gravity;
  if (!pressing(keyStates, SDL_SCANCODE_UP) &&
      playerState.jumpState == JumpState::started) {
    playerState.jumpState = JumpState::released;
    if (playerState.object.velocity.vertical < 0)
      playerState.object.velocity.vertical = {0, 1};
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
  const RationalDistance gravity{1, 4};
  const auto groundFriction{1};
  const auto playerMaxHorizontalSpeed{4};
  const auto playerJumpAcceleration{-6};
  const auto playerRunAcceleration{2};
  PlayerState playerState{
      {Rectangle{Point{0, topEdge(floorRectangle) - playerHeight}, playerWidth,
                 playerHeight},
       Velocity{{0, 1}, 0}},
      JumpState::grounded};
  MovingObject enemy{{Point{140, topEdge(floorRectangle) - enemyHeight},
                      enemyWidth, enemyHeight},
                     Velocity{{0, 1}, 0}};
  const Rectangle blockRectangle{Point{256, 144}, 15, 15};
  const auto pipeHeight{40};
  const Rectangle pipeRectangle{
      Point{448, topEdge(floorRectangle) - pipeHeight}, 30, pipeHeight};
  Rectangle backgroundSourceRectangle{Point{0, 0}, cameraWidth, cameraHeight};
  auto playing{true};
  while (playing) {
    flushEvents(playing);
    playerState = handleVerticalCollisions(
        applyVerticalForces(applyHorizontalForces(playerState, groundFriction,
                                                  playerMaxHorizontalSpeed,
                                                  playerRunAcceleration),
                            playerJumpAcceleration, gravity),
        {blockRectangle, pipeRectangle}, {blockRectangle}, floorRectangle);
    playerState.object = handleHorizontalCollisions(
        {playerState.object.rectangle, playerState.object.velocity},
        {blockRectangle, pipeRectangle}, {blockRectangle, pipeRectangle},
        levelRectangle);
    playerState = applyVelocity(playerState);
    if (leftEdge(playerState.object.rectangle) < leftEdge(enemy.rectangle))
      enemy.velocity.horizontal = -1;
    else if (leftEdge(playerState.object.rectangle) > leftEdge(enemy.rectangle))
      enemy.velocity.horizontal = 1;
    else
      enemy.velocity.horizontal = 0;
    enemy = handleHorizontalCollisions(enemy, {pipeRectangle}, {pipeRectangle},
                                       levelRectangle);
    enemy.rectangle = applyHorizontalVelocity(enemy.rectangle, enemy.velocity);
    backgroundSourceRectangle =
        shiftBackground(backgroundSourceRectangle, backgroundSourceWidth,
                        playerState.object.rectangle, cameraWidth);
    present(rendererWrapper, backgroundTextureWrapper,
            backgroundSourceRectangle, pixelScale,
            {Point{0, 0}, cameraWidth, cameraHeight});
    present(rendererWrapper, enemyTextureWrapper, enemySourceRect, pixelScale,
            shiftHorizontally(enemy.rectangle,
                              -leftEdge(backgroundSourceRectangle)));
    present(rendererWrapper, playerTextureWrapper, playerSourceRect, pixelScale,
            shiftHorizontally(playerState.object.rectangle,
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
