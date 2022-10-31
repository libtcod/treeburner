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

  float x{};
  float y{};

  Entity() = default;
  Entity(int px, int py) noexcept : x{gsl::narrow_cast<float>(px)}, y(gsl::narrow_cast<float>(py)) {}
  Entity(float px, float py) noexcept : x{px}, y{py} {}
  // get subcell coordinates
  int getSubX() const noexcept { return gsl::narrow_cast<int>(x * 2); }
  int getSubY() const noexcept { return gsl::narrow_cast<int>(y * 2); }
  void setPos(int new_x, int new_y) noexcept {
    x = gsl::narrow_cast<float>(new_x);
    y = gsl::narrow_cast<float>(new_y);
  }
  void setPos(float new_x, float new_y) noexcept {
    x = new_x;
    y = new_y;
  }
  Entity& addDir(Direction d) noexcept {
    static constexpr auto x_dirs = std::array{0, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1};
    static constexpr auto y_dirs = std::array{0, 0, 0, -1, 1, 0, 0, -1, -1, 1, 1};
    x += x_dirs[d];
    y += y_dirs[d];
    return *this;
  }
  static Direction movementToDir(int xFrom, int yFrom, int xTo, int yTo) {
    static constexpr Direction dirs[3][3] = {{NW, NORTH, NE}, {WEST, NONE, EAST}, {SW, SOUTH, SE}};
    return dirs[yTo - yFrom + 1][xTo - xFrom + 1];
  }
  float squaredDistance(const Entity& p) const { return (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y); }
  bool isOnScreen() const;
  float distance(const Entity& p) const;
  float fastInvDistance(const Entity& p) const { return fastInvSqrt(squaredDistance(p)); }
  static float fastInvSqrt(float n);
};

// a rectangular zone in the world
class Rect : public Entity {
 public:
  int w{};
  int h{};

  Rect() = default;
  Rect(int x, int y, int pw, int ph) noexcept
      : Entity(gsl::narrow_cast<float>(x), gsl::narrow_cast<float>(y)), w(pw), h(ph) {}
  Rect(float x, float y, int pw, int ph) noexcept : Entity{x, y}, w{pw}, h{ph} {}
  bool pointInside(float px, float py) const noexcept { return (px >= x && py >= y && px < x + w && py < y + h); }
  bool pointInside(const Entity& pt) const noexcept { return pointInside(pt.x, pt.y); }
  bool isIntersecting(const Rect& r) const noexcept {
    return !(r.x > x + w || r.x + r.w < x || r.y > y + h || r.y + r.h < y);
  }
  // smallest rectangle containing this and r
  void merge(const Rect& r) noexcept {
    float minx = std::min(x, r.x);
    float maxx = std::max(x + w, r.x + r.w);
    float miny = std::min(y, r.y);
    float maxy = std::max(y + h, r.y + r.h);
    x = minx;
    w = gsl::narrow_cast<int>(maxx - minx);
    y = miny;
    h = gsl::narrow_cast<int>(maxy - miny);
  }
  // intersection of this and r
  void intersect(const Rect& r) noexcept {
    float minx = std::max(x, r.x);
    float maxx = std::min(x + w, r.x + r.w);
    float miny = std::max(y, r.y);
    float maxy = std::min(y + h, r.y + r.h);
    x = minx;
    y = miny;
    w = gsl::narrow_cast<int>(maxx - minx);
    h = gsl::narrow_cast<int>(maxy - miny);
  }
};

// entity with dynamic pos and speed
class DynamicEntity : public Entity {
 public:
  float dx{}, dy{};  // movement direction (unit vector)
  float duration{};  // movement duration in seconds
  float speed{};  // speed in cells/sec
  DynamicEntity() = default;
};
}  // namespace base
