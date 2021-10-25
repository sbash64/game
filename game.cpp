#include <sbash64/game/game.hpp>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <vector>

namespace sbash64::game {
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

[[nodiscard]] auto
HorizontalCollision::distanceFirstExceedsSecondParallelToSurface(
    Rectangle a, Rectangle b) const -> distance_type {
  return distanceFirstExceedsSecondVertically(a, b);
}

[[nodiscard]] auto
HorizontalCollision::applyVelocityNormalToSurface(MovingObject a) const
    -> Rectangle {
  return applyHorizontalVelocity(a);
}

[[nodiscard]] auto
HorizontalCollision::applyVelocityParallelToSurface(MovingObject a) const
    -> Rectangle {
  return applyVerticalVelocity(a);
}

[[nodiscard]] auto
HorizontalCollision::headingTowardUpperBoundary(Velocity a) const -> bool {
  return round(a.vertical) > 0;
}

[[nodiscard]] auto
HorizontalCollision::headingTowardLowerBoundary(Velocity a) const -> bool {
  return isNegative(round(a.vertical));
}

[[nodiscard]] auto HorizontalCollision::surfaceRelativeSlope(Velocity a) const
    -> RationalDistance {
  return RationalDistance{std::abs(a.horizontal), round(a.vertical)};
}

[[nodiscard]] auto
VerticalCollision::distanceFirstExceedsSecondParallelToSurface(
    Rectangle a, Rectangle b) const -> distance_type {
  return distanceFirstExceedsSecondHorizontally(a, b);
}

[[nodiscard]] auto
VerticalCollision::applyVelocityNormalToSurface(MovingObject a) const
    -> Rectangle {
  return applyVerticalVelocity(a);
}

[[nodiscard]] auto
VerticalCollision::applyVelocityParallelToSurface(MovingObject a) const
    -> Rectangle {
  return applyHorizontalVelocity(a);
}

[[nodiscard]] auto
VerticalCollision::headingTowardUpperBoundary(Velocity a) const -> bool {
  return a.horizontal > 0;
}

[[nodiscard]] auto
VerticalCollision::headingTowardLowerBoundary(Velocity a) const -> bool {
  return isNegative(a.horizontal);
}

[[nodiscard]] auto VerticalCollision::surfaceRelativeSlope(Velocity a) const
    -> RationalDistance {
  return RationalDistance{std::abs(round(a.vertical)), a.horizontal};
}

[[nodiscard]] auto CollisionFromBelow::distancePenetrates(
    MovingObject movingObject, Rectangle stationaryObjectRectangle) const
    -> distance_type {
  return distanceFirstExceedsSecondVertically(movingObject.rectangle,
                                              stationaryObjectRectangle);
}

[[nodiscard]] auto CollisionFromAbove::distancePenetrates(
    MovingObject movingObject, Rectangle stationaryObjectRectangle) const
    -> distance_type {
  return distanceFirstExceedsSecondVertically(stationaryObjectRectangle,
                                              movingObject.rectangle);
}

[[nodiscard]] auto CollisionFromRight::distancePenetrates(
    MovingObject movingObject, Rectangle stationaryObjectRectangle) const
    -> distance_type {
  return distanceFirstExceedsSecondHorizontally(movingObject.rectangle,
                                                stationaryObjectRectangle);
}

[[nodiscard]] auto
CollisionFromLeft::distancePenetrates(MovingObject movingObject,
                                      Rectangle stationaryObjectRectangle) const
    -> distance_type {
  return distanceFirstExceedsSecondHorizontally(stationaryObjectRectangle,
                                                movingObject.rectangle);
}

static auto passesThroughTowardUpperBoundary(
    MovingObject movingObject, Rectangle stationaryObjectRectangle,
    const CollisionDirection &direction, const CollisionAxis &axis) -> bool {
  return axis.headingTowardUpperBoundary(movingObject.velocity) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             axis.applyVelocityParallelToSurface(movingObject),
             stationaryObjectRectangle)) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             stationaryObjectRectangle, movingObject.rectangle)) &&
         axis.surfaceRelativeSlope(movingObject.velocity) >
             RationalDistance{
                 -(direction.distancePenetrates(movingObject,
                                                stationaryObjectRectangle) +
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
             axis.applyVelocityParallelToSurface(movingObject))) &&
         isNonnegative(axis.distanceFirstExceedsSecondParallelToSurface(
             movingObject.rectangle, stationaryObjectRectangle)) &&
         axis.surfaceRelativeSlope(movingObject.velocity) <
             RationalDistance{
                 direction.distancePenetrates(movingObject,
                                              stationaryObjectRectangle) +
                     1,
                 axis.distanceFirstExceedsSecondParallelToSurface(
                     movingObject.rectangle, stationaryObjectRectangle) +
                     1};
}

auto passesThrough(MovingObject movingObject,
                   Rectangle stationaryObjectRectangle,
                   const CollisionDirection &direction,
                   const CollisionAxis &axis) -> bool {
  if (isNonnegative(direction.distancePenetrates(movingObject,
                                                 stationaryObjectRectangle)) ||
      isNegative(direction.distancePenetrates(
          {axis.applyVelocityNormalToSurface(movingObject),
           movingObject.velocity},
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

auto handleVerticalCollisions(
    PlayerState playerState,
    const std::vector<Rectangle> &collisionFromBelowCandidates,
    const std::vector<Rectangle> &collisionFromAboveCandidates,
    const Rectangle &floorRectangle) -> PlayerState {
  for (const auto candidate : sortByTopEdge(collisionFromBelowCandidates))
    if (passesThrough(playerState.object, candidate, CollisionFromBelow{},
                      VerticalCollision{}))
      return onPlayerHitGround(playerState, topEdge(candidate));
  if (isNonnegative(distanceFirstExceedsSecondVertically(
          applyVerticalVelocity(playerState.object), floorRectangle)))
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

static auto collideHorizontally(MovingObject object, distance_type leftEdge)
    -> MovingObject {
  object.velocity.horizontal = 0;
  object.rectangle.origin.x = leftEdge;
  return object;
}

auto handleHorizontalCollisions(
    MovingObject object,
    const std::vector<Rectangle> &collisionFromRightCandidates,
    const std::vector<Rectangle> &collisionFromLeftCandidates,
    const Rectangle &levelRectangle) -> MovingObject {
  for (const auto candidate : sortByLeftEdge(collisionFromRightCandidates))
    if (passesThrough(object, candidate, CollisionFromRight{},
                      HorizontalCollision{}))
      return collideHorizontally(object,
                                 leftEdge(candidate) - object.rectangle.width);
  if (isNonnegative(rightEdge(applyHorizontalVelocity(object)) -
                    rightEdge(levelRectangle)))
    return collideHorizontally(object, rightEdge(levelRectangle) -
                                           object.rectangle.width);

  for (const auto candidate : sortByRightEdge(collisionFromLeftCandidates))
    if (passesThrough(object, candidate, CollisionFromLeft{},
                      HorizontalCollision{}))
      return collideHorizontally(object, rightEdge(candidate) + 1);
  if (isNonnegative(leftEdge(levelRectangle) -
                    leftEdge(applyHorizontalVelocity(object))))
    return collideHorizontally(object, leftEdge(levelRectangle) + 1);
  return object;
}
} // namespace sbash64::game
