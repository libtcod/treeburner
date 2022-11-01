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
#include "util/cellular.hpp"

#include <algorithm>
#include <utility>

#include "main.hpp"

// cells value :
// 1 : wall
// 0 : ground

namespace util {
CellularAutomata::CellularAutomata(TCODMap* map) : CellularAutomata{map->getWidth(), map->getHeight()} {
  for (int py = 0; py < h_; ++py) {
    for (int px = 0; px < w_; ++px) {
      data_[px + py * w_] = !map->isWalkable(px, py);
    }
  }
}

void CellularAutomata::seal() {
  for (int px = 0; px < w_; ++px) {
    data_[px] = 1;
    data_[px + (h_ - 1) * w_] = 1;
  }
  for (int py = 0; py < h_; ++py) {
    data_[py * w_] = 1;
    data_[w_ - 1 + py * w_] = 1;
  }
}

void CellularAutomata::apply(TCODMap* map) {
  for (int py = min_y_; py <= max_x_; ++py) {
    for (int px = min_x_; px <= max_x_; ++px) {
      if (data_[px + py * w_])
        map->setProperties(px, py, false, false);
      else
        map->setProperties(px, py, true, true);
    }
  }
}

void CellularAutomata::randomize(int per) {
  for (int px = min_x_; px <= max_x_; ++px) {
    for (int py = min_y_; py <= max_x_; ++py) {
      if (rng->getInt(0, 100) < per)
        data_[px + py * w_] = 1;
      else
        data_[px + py * w_] = 0;
    }
  }
  for (int px = min_x_; px <= max_x_; ++px) {
    data_[px] = 1;
    data_[px + w_ * (h_ - 1)] = 1;
  }
  for (int py = min_y_; py <= max_x_; ++py) {
    data_[py * w_] = 1;
    data_[w_ - 1 + w_ * py] = 1;
  }
}

void CellularAutomata::generate(CAFunc func, int nbLoops, void* userData) {
  auto data2 = data_;
  for (int l = 0; l < nbLoops; ++l) {
    for (int py = min_y_ + 1; py < max_y_; ++py) {
      for (int px = min_x_ + 1; px < max_x_; ++px) {
        if ((this->*func)(px, py, userData))
          data2[px + py * w_] = 1;
        else
          data2[px + py * w_] = 0;
      }
    }
    data_ = std::move(data2);
  }
}

// number of walls around x,y
int CellularAutomata::count(int x, int y, int range) {
  int pminx = x - range;
  int pminy = y - range;
  int pmaxx = x + range;
  int pmaxy = y + range;
  // count in map walls
  int c = 0;
  for (int py = pminy; py <= pmaxy; ++py) {
    for (int px = pminx; px <= pmaxx; ++px) {
      if (IN_RECTANGLE(px, py, w_, h_)) {
        if (px != x || py != y) c += data_[px + py * w_];
      } else
        ++c;
    }
  }
  return c;
}

void CellularAutomata::connect() {
  // find an empty cell
  int i = 0;
  while (i < w_ * h_ && data_[i]) i++;
  if (i == w_ * h_) return;  // no empty cell
  while (true) {
    // floodfill from this empty cell with value 2
    TCODList<int> cells;
    cells.push(i);
    while (!cells.isEmpty()) {
      int j = cells.pop();
      data_[j] = 2;
      // west cell
      if ((j % w_) > 0 && data_[j - 1] == 0) cells.push(j - 1);
      // east cell
      if ((j % w_) < w_ - 1 && data_[j + 1] == 0) cells.push(j + 1);
      // north cell
      if (j >= w_ && data_[j - w_] == 0) cells.push(j - w_);
      // south cell
      if (j < w_ * (h_ - 1) && data_[j + w_] == 0) cells.push(j + w_);
      // north-west cell
      if ((j % w_) > 0 && j >= w_ && data_[j - w_ - 1] == 0) cells.push(j - w_ - 1);
      // south-west cell
      if (j < w_ * (h_ - 1) && (j % w_) > 0 && data_[j + w_ - 1] == 0) cells.push(j + w_ - 1);
      // north-east cell
      if ((j % w_) < w_ - 1 && j >= w_ && data_[j - w_ + 1] == 0) cells.push(j - w_ + 1);
      // south-east cell
      if (j < w_ * (h_ - 1) && (j % w_) < w_ - 1 && data_[j + w_ + 1] == 0) cells.push(j + w_ + 1);
    }
    while (i < w_ * h_ && data_[i]) i++;
    if (i == w_ * h_) break;  // no empty cell
    // find the nearest 2 cell
    int mindist = 99999;
    int bx = 0, by = 0;
    int cx = i % w_;
    int cy = i / w_;
    for (int y = 0; y < h_; ++y) {
      for (int x = 0; x < w_; ++x) {
        if (data_[x + y * w_] == 2) {
          int dist = SQRDIST(x, y, cx, cy);
          if (dist < mindist) {
            mindist = dist;
            bx = x;
            by = y;
          }
        }
      }
    }
    // link cx,cy to bx,by
    while (data_[cx + cy * w_] != 2 && cx > bx) {
      data_[cx + cy * w_] = 2;
      cx--;
    }
    while (data_[cx + cy * w_] != 2 && cx < bx) {
      data_[cx + cy * w_] = 2;
      cx++;
    }
    while (data_[cx + cy * w_] != 2 && cy < by) {
      data_[cx + cy * w_] = 2;
      cy++;
    }
    while (data_[cx + cy * w_] != 2 && cy > by) {
      data_[cx + cy * w_] = 2;
      cy--;
    }
  }
  // replace all 2 by 0
  i = 0;
  while (i < w_ * h_) {
    if (data_[i] == 2) data_[i] = 0;
    i++;
  }
}

bool CellularAutomata::CAFunc_cave(int x, int y, void*) { return count(x, y, 1) >= 5 || count(x, y, 2) <= 2; }

bool CellularAutomata::CAFunc_cave2(int x, int y, void*) { return count(x, y, 1) >= 5; }

bool CellularAutomata::CAFunc_removeInnerWalls(int x, int y, void*) { return data_[x + y * w_] && count(x, y, 2) < 24; }

bool CellularAutomata::CAFunc_roundCorners(int x, int y, void*) {
  int cnt = count(x, y, 1);
  if (cnt == 5) return 1;
  if (cnt == 3) return 0;
  return data_[x + y * w_];
}

bool CellularAutomata::CAFunc_cleanIsolatedWalls(int x, int y, void*) {
  return data_[x + y * w_] && count(x, y, 1) >= 2;
}

bool CellularAutomata::CAFunc_dig(int x, int y, void*) {
  return (
      data_[x + y * w_]  // empty cells stays empty
      && count(x, y, 1) == 8);  // rocks adjacent to empty gets empty
}

void CellularAutomata::setRange(int minx, int miny, int maxx, int maxy) {
  this->min_x_ = std::max(0, minx);
  this->min_y_ = std::max(0, miny);
  this->max_x_ = std::min(w_ - 1, maxx);
  this->max_y_ = std::min(h_ - 1, maxy);
}

CellularAutomata::CellularAutomata(CellularAutomata& c1, CellularAutomata& c2, float morphCoef)
    : CellularAutomata{c1.w_, c1.h_} {
  for (int y = 0; y < h_; ++y) {
    for (int x = 0; x < w_; ++x) {
      data_[x + y * w_] =
          (uint8_t)(morphCoef * (c1.data_[x + y * w_] * 8) + (1.0 - morphCoef) * (c2.data_[x + y * w_] * 8));
    }
  }
  auto data2 = std::vector<uint8_t>(w_ * h_);
  for (int y = 0; y < h_; ++y) {
    for (int x = 0; x < w_; ++x) {
      if (((int)data_[x + y * w_]) * 5 + count(x, y, 1) / 2 >= 40) data2[x + y * w_] = 1;
    }
  }
  data_ = std::move(data2);
}
}  // namespace util
