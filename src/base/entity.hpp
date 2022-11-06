/*
 * Copyright (c) 2009 Jice
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Jice may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Jice ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Jice BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#include <algorithm>
#include <gsl/gsl>

namespace base {
// a position in the world
class Entity {
 public:
  enum Direction { NONE = 0, UP, DOWN, NORTH, SOUTH, EAST, WEST, NE, NW, SE, SW };

  Entity() = default;
  Entity(int x, int y) noexcept : x_{gsl::narrow_cast<float>(x)}, y_(gsl::narrow_cast<float>(y)) {}
  Entity(float x, float y) noexcept : x_{x}, y_{y} {}
  // get subcell coordinates
  int getSubX() const noexcept { return gsl::narrow_cast<int>(x_ * 2); }
  int getSubY() const noexcept { return gsl::narrow_cast<int>(y_ * 2); }
  void setPos(int x, int y) noexcept {
    x_ = gsl::narrow_cast<float>(x);
    y_ = gsl::narrow_cast<float>(y);
  }
  void setPos(float x, float y) noexcept {
    x_ = x;
    y_ = y;
  }
  Entity& addDir(Direction d) noexcept {
    static constexpr auto x_dirs = std::array{0, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1};
    static constexpr auto y_dirs = std::array{0, 0, 0, -1, 1, 0, 0, -1, -1, 1, 1};
    x_ += x_dirs[d];
    y_ += y_dirs[d];
    return *this;
  }
  static Direction movementToDir(int xFrom, int yFrom, int xTo, int yTo) {
    static constexpr Direction dirs[3][3] = {{NW, NORTH, NE}, {WEST, NONE, EAST}, {SW, SOUTH, SE}};
    return dirs[yTo - yFrom + 1][xTo - xFrom + 1];
  }
  float squaredDistance(const Entity& p) const noexcept {
    return (p.x_ - x_) * (p.x_ - x_) + (p.y_ - y_) * (p.y_ - y_);
  }
  bool isOnScreen() const;
  float distance(const Entity& p) const;
  float fastInvDistance(const Entity& p) const { return fastInvSqrt(squaredDistance(p)); }
  static float fastInvSqrt(float n);

  float x_{};
  float y_{};
};

// a rectangular zone in the world
class Rect : public Entity {
 public:
  Rect() = default;
  Rect(int x, int y, int width, int height) noexcept
      : Entity(gsl::narrow_cast<float>(x), gsl::narrow_cast<float>(y)), w_{width}, h_{height} {}
  Rect(float x, float y, int width, int height) noexcept : Entity{x, y}, w_{width}, h_{height} {}
  bool pointInside(float px, float py) const noexcept { return (px >= x_ && py >= y_ && px < x_ + w_ && py < y_ + h_); }
  bool pointInside(const Entity& pt) const noexcept { return pointInside(pt.x_, pt.y_); }
  bool isIntersecting(const Rect& r) const noexcept {
    return !(r.x_ > x_ + w_ || r.x_ + r.w_ < x_ || r.y_ > y_ + h_ || r.y_ + r.h_ < y_);
  }
  // smallest rectangle containing this and r
  void merge(const Rect& r) noexcept {
    const float min_x = std::min(x_, r.x_);
    const float max_x = std::max(x_ + w_, r.x_ + r.w_);
    const float min_y = std::min(y_, r.y_);
    const float max_y = std::max(y_ + h_, r.y_ + r.h_);
    x_ = min_x;
    w_ = gsl::narrow_cast<int>(max_x - min_x);
    y_ = min_y;
    h_ = gsl::narrow_cast<int>(max_y - min_y);
  }
  // intersection of this and r
  void intersect(const Rect& r) noexcept {
    const float min_x = std::max(x_, r.x_);
    const float max_x = std::min(x_ + w_, r.x_ + r.w_);
    const float min_y = std::max(y_, r.y_);
    const float max_y = std::min(y_ + h_, r.y_ + r.h_);
    x_ = min_x;
    y_ = min_y;
    w_ = gsl::narrow_cast<int>(max_x - min_x);
    h_ = gsl::narrow_cast<int>(max_y - min_y);
  }

  int w_{};
  int h_{};
};

// entity with dynamic pos and speed
class DynamicEntity : public Entity {
 public:
  DynamicEntity() = default;
  float dx_{}, dy_{};  // movement direction (unit vector)
  float duration_{};  // movement duration in seconds
  float speed_{};  // speed in cells/sec
};
}  // namespace base
