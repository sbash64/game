#ifndef SBASH64_GAME_GAME_HPP_
#define SBASH64_GAME_GAME_HPP_

#include <algorithm>
#include <cstdlib>
#include <limits>
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

enum class JumpState { grounded, started, released };

enum class DirectionFacing { left, right };

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
  DirectionFacing directionFacing;
};

constexpr auto operator-(Velocity a) -> Velocity {
  return {-a.vertical, -a.horizontal};
}

constexpr auto shiftHorizontally(Rectangle a, distance_type b) -> Rectangle {
  a.origin.x += b;
  return a;
}

constexpr auto applyHorizontalVelocity(MovingObject a) -> Rectangle {
  return shiftHorizontally(a.rectangle, a.velocity.horizontal);
}

constexpr auto applyVerticalVelocity(MovingObject a) -> Rectangle {
  a.rectangle.origin.y += round(a.velocity.vertical);
  return a.rectangle;
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
  [[nodiscard]] virtual auto distancePenetrates(MovingObject, Rectangle) const
      -> distance_type = 0;
};

class CollisionAxis {
public:
  [[nodiscard]] virtual auto
      distanceFirstExceedsSecondParallelToSurface(Rectangle, Rectangle) const
      -> distance_type = 0;
  [[nodiscard]] virtual auto applyVelocityNormalToSurface(MovingObject) const
      -> Rectangle = 0;
  [[nodiscard]] virtual auto applyVelocityParallelToSurface(MovingObject) const
      -> Rectangle = 0;
  [[nodiscard]] virtual auto headingTowardUpperBoundary(Velocity) const
      -> bool = 0;
  [[nodiscard]] virtual auto headingTowardLowerBoundary(Velocity) const
      -> bool = 0;
  [[nodiscard]] virtual auto surfaceRelativeSlope(Velocity) const
      -> RationalDistance = 0;
};

class HorizontalCollision : public CollisionAxis {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> distance_type override;
  [[nodiscard]] auto applyVelocityNormalToSurface(MovingObject a) const
      -> Rectangle override;
  [[nodiscard]] auto applyVelocityParallelToSurface(MovingObject a) const
      -> Rectangle override;
  [[nodiscard]] auto headingTowardUpperBoundary(Velocity a) const
      -> bool override;
  [[nodiscard]] auto headingTowardLowerBoundary(Velocity a) const
      -> bool override;
  [[nodiscard]] auto surfaceRelativeSlope(Velocity a) const
      -> RationalDistance override;
};

class VerticalCollision : public CollisionAxis {
public:
  [[nodiscard]] auto
  distanceFirstExceedsSecondParallelToSurface(Rectangle a, Rectangle b) const
      -> distance_type override;
  [[nodiscard]] auto applyVelocityNormalToSurface(MovingObject a) const
      -> Rectangle override;
  [[nodiscard]] auto applyVelocityParallelToSurface(MovingObject a) const
      -> Rectangle override;
  [[nodiscard]] auto headingTowardUpperBoundary(Velocity a) const
      -> bool override;
  [[nodiscard]] auto headingTowardLowerBoundary(Velocity a) const
      -> bool override;
  [[nodiscard]] auto surfaceRelativeSlope(Velocity a) const
      -> RationalDistance override;
};

class CollisionFromBelow : public CollisionDirection {
public:
  [[nodiscard]] auto
  distancePenetrates(MovingObject movingObject,
                     Rectangle stationaryObjectRectangle) const
      -> distance_type override;
};

class CollisionFromAbove : public CollisionDirection {
public:
  [[nodiscard]] auto
  distancePenetrates(MovingObject movingObject,
                     Rectangle stationaryObjectRectangle) const
      -> distance_type override;
};

class CollisionFromRight : public CollisionDirection {
public:
  [[nodiscard]] auto
  distancePenetrates(MovingObject movingObject,
                     Rectangle stationaryObjectRectangle) const
      -> distance_type override;
};

class CollisionFromLeft : public CollisionDirection {
public:
  [[nodiscard]] auto
  distancePenetrates(MovingObject movingObject,
                     Rectangle stationaryObjectRectangle) const
      -> distance_type override;
};

auto passesThrough(MovingObject movingObject,
                   Rectangle stationaryObjectRectangle,
                   const CollisionDirection &direction,
                   const CollisionAxis &axis) -> bool;

auto handleVerticalCollisions(
    PlayerState playerState,
    const std::vector<Rectangle> &collisionFromBelowCandidates,
    const std::vector<Rectangle> &collisionFromAboveCandidates,
    const Rectangle &floorRectangle) -> PlayerState;

auto handleHorizontalCollisions(
    MovingObject object,
    const std::vector<Rectangle> &collisionFromRightCandidates,
    const std::vector<Rectangle> &collisionFromLeftCandidates,
    const Rectangle &levelRectangle) -> MovingObject;

auto shiftBackground(Rectangle backgroundSourceRectangle,
                     distance_type backgroundSourceWidth,
                     const Rectangle &playerRectangle,
                     distance_type cameraWidth) -> Rectangle;

auto applyVelocity(MovingObject object) -> MovingObject;
} // namespace sbash64::game

#endif
