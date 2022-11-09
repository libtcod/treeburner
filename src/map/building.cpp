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
#include "map/building.hpp"

#include <math.h>
#include <stdio.h>

#include "main.hpp"

namespace map {
Building* Building::generate(int width, int height, int nbRooms, TCODRandom* rng) {
  Building* ret = generateWallsOnly(width, height, nbRooms, rng);
  ret->placeRandomDoor(rng);
  for (int room = 0; room < nbRooms; room++) {
    ret->placeRandomWindow(rng);
  }
  return ret;
}

Building* Building::generateWallsOnly(int width, int height, int nbRooms, TCODRandom* rng) {
  Building* ret = new Building(width, height);
  int oldrx = 0, oldry = 0, oldrw = 0, oldrh = 0;
  if (!rng) rng = TCODRandom::getInstance();
  int buildingSurf = width * height;
  int averageRoomWidth = 0, averageRoomHeight = 0;
  if (buildingSurf < 150) {
    averageRoomWidth = width / nbRooms;
    averageRoomHeight = height / nbRooms;
  } else {
    int averageRoomSurf = width * height / nbRooms;
    averageRoomWidth = averageRoomHeight = (int)(0.5f + sqrtf(averageRoomSurf));
  }
  // draw random rectangular rooms
  for (int room = 0; room < nbRooms; room++) {
    int rx, ry, rw, rh;
    rw = rng->getInt(averageRoomWidth, std::min(width, averageRoomWidth * 2));
    rh = rng->getInt(averageRoomHeight, std::min(height, averageRoomHeight * 2));
    if (room == 0) {
      rx = rng->getInt(0, width - rw);
      ry = rng->getInt(0, height - rh);
    } else {
      // rooms should not be disjoint
      rx = rng->getInt(oldrx - rw, oldrx + oldrw);
      ry = rng->getInt(oldry - rh, oldry + oldrh);
      rx = std::clamp(rx, 0, width - rw);
      ry = std::clamp(ry, 0, height - rh);
    }
    // fill room with floor
    for (int rry = ry; rry < ry + rh; rry++) {
      for (int rrx = rx; rrx < rx + rw; rrx++) {
        ret->map[rrx + rry * width] = BUILDING_FLOOR;
      }
    }
    oldrx = rx;
    oldry = ry;
    oldrw = rw;
    oldrh = rh;
  }
  ret->buildExternalWalls();
  return ret;
}

void Building::placeRandomDoor(TCODRandom* rng) {
  doorx = rng->getInt(0, w_ - 1);
  doory = rng->getInt(0, h_ - 1);
  while (map[doorx + doory * w_] < BUILDING_WALL_N || map[doorx + doory * w_] > BUILDING_WALL_W) {
    // not a door-hosting wall
    doorx++;
    if (doorx == w_) {
      doorx = 0;
      doory++;
      if (doory == h_) doory = 0;
    }
  }
  map[doorx + doory * w_] = BUILDING_DOOR;
}

#define IS_FLAT_WALL(x) ((x) >= BUILDING_WALL_N && (x) <= BUILDING_WALL_W)
void Building::placeRandomWindow(TCODRandom* rng) {
  int winx = rng->getInt(0, w_ - 1);
  int winy = rng->getInt(0, h_ - 1);
  bool ok = false;
  int count = w_ * h_;
  bool horiz = false;
  while (!ok && count > 0) {
    int cell = map[winx + winy * w_];
    while (count > 0 && !IS_FLAT_WALL(cell)) {
      // not a window-hosting wall
      count--;
      winx++;
      if (winx == w_) {
        winx = 0;
        winy++;
        if (winy == h_) winy = 0;
      }
      cell = map[winx + winy * w_];
    }
    // window should not be adjacent to door or corner
    horiz = ((cell & 1) == 0);
    int before = BUILDING_NONE;
    int after = BUILDING_NONE;
    if (horiz) {
      if (winx > 0) before = map[winx - 1 + winy * w_];
      if (winx < w_ - 1) after = map[winx + 1 + winy * w_];
    } else {
      if (winy > 0) before = map[winx + (winy - 1) * w_];
      if (winy < h_ - 1) after = map[winx + (winy + 1) * w_];
    }
    if (IS_FLAT_WALL(before) && IS_FLAT_WALL(after))
      ok = true;
    else {
      count--;
      winx++;
      if (winx == w_) {
        winx = 0;
        winy++;
        if (winy == h_) winy = 0;
      }
    }
  }
  if (!ok) return;  // no place found
  map[winx + winy * w_] = horiz ? BUILDING_WINDOW_H : BUILDING_WINDOW_V;
}

// scan the map. put walls at floor/none borders
void Building::buildExternalWalls() {
  for (int y = 0; y < h_; y++) {
    for (int x = 0; x < w_; x++) {
      if (map[x + y * w_] == BUILDING_FLOOR) {
        // check if there are adjacent outdoor cells
        enum BuildDir { BD_NW = 1, BD_N = 2, BD_NE = 4, BD_E = 8, BD_SE = 16, BD_S = 32, BD_SW = 64, BD_W = 128 };
        int bd = 0;
        static int xdir[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
        static int ydir[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
        // find direction that contain outdoor cell
        for (int dir = 0; dir < 8; dir++) {
          int dx = x + xdir[dir];
          int dy = y + ydir[dir];
          // flag outdoor directions
          if (!IN_RECTANGLE(dx, dy, w_, h_) || map[dx + dy * w_] == BUILDING_NONE) bd |= (1 << dir);
        }
        if (bd != 0) {
          // oudoor cells found. create an external wall
          int wall = map[x + y * w_];
#define HAS_FLAG(f, f2) (((f) & (f2)) == (f2))
#define IS_NW(f) HAS_FLAG(f, BD_W | BD_NW | BD_N)
#define IS_NE(f) HAS_FLAG(f, BD_N | BD_NE | BD_E)
#define IS_SE(f) HAS_FLAG(f, BD_E | BD_SE | BD_S)
#define IS_SW(f) HAS_FLAG(f, BD_S | BD_SW | BD_W)
#define IS_N(f) HAS_FLAG(f, BD_N)
#define IS_E(f) HAS_FLAG(f, BD_E)
#define IS_S(f) HAS_FLAG(f, BD_S)
#define IS_W(f) HAS_FLAG(f, BD_W)
          if (IS_NW(bd))
            wall = BUILDING_WALL_NW;
          else if (IS_NE(bd))
            wall = BUILDING_WALL_NE;
          else if (IS_SE(bd))
            wall = BUILDING_WALL_SE;
          else if (IS_SW(bd))
            wall = BUILDING_WALL_SW;
          else if (bd == BD_NW)
            wall = BUILDING_WALL_SE;
          else if (bd == BD_NE)
            wall = BUILDING_WALL_SW;
          else if (bd == BD_SE)
            wall = BUILDING_WALL_NW;
          else if (bd == BD_SW)
            wall = BUILDING_WALL_NE;
          else if (IS_N(bd))
            wall = BUILDING_WALL_N;
          else if (IS_E(bd))
            wall = BUILDING_WALL_E;
          else if (IS_S(bd))
            wall = BUILDING_WALL_S;
          else if (IS_W(bd))
            wall = BUILDING_WALL_W;
          map[x + y * w_] = wall;
        }
      }
    }
  }
}

bool Building::getFreeFloor(int* fx, int* fy) {
  int ffx = rng->getInt(0, w_ - 1);
  int ffy = rng->getInt(0, h_ - 1);
  int count = w_ * h_;
  while (count > 0 && map[ffx + ffy * w_] != BUILDING_FLOOR) {
    ffx++;
    count--;
    if (ffx == w_) {
      ffx = 0;
      ffy++;
      if (ffy == h_) ffy = 0;
    }
  }
  if (count == 0) return false;
  *fx = ffx;
  *fy = ffy;
  return true;
}

void Building::setHuntingHide(map::Dungeon* dungeon) {
  int ix, iy;
  /*
  if (getFreeFloor(&ix,&iy)) {
          Item *item=Item::getItem(ITEM_BAG,x+ix,y+iy);
          dungeon->addItem(item);
          map[ix+iy*w] = BUILDING_ITEM;
  }
  */
  if (getFreeFloor(&ix, &iy)) {
    item::Item* chest = item::Item::getItem("chest", x_ + ix, y_ + iy);
    dungeon->addItem(chest);
    map[ix + iy * w_] = BUILDING_ITEM;
    // fill the chest

    item::Item* item = item::Item::getItem("short bronze blade", 0, 0);
    // Item *knife=Item::getRandomWeapon(ITEM_KNIFE,ITEM_CLASS_STANDARD);
    // knife->name = "hunting knife";
    item->name_ = "knife blade";
    item->adjective_ = "hunting";
    item->putInContainer(chest);

    item = item::Item::getItem("bottle", 0, 0);
    item->putInContainer(chest);
    item->name_ = "empty bottle";
    item->an_ = true;

    item = item::Item::getItem("linen thread", 0, 0);
    item->count_ = 2;
    item->putInContainer(chest);

    item = item::Item::getItem("bone hook", 0, 0);
    item->putInContainer(chest);
  }
}

void Building::applyTo(map::Dungeon* dungeon, int dungeonDoorx, int dungeonDoory, bool cityWalls) {
  static TCODColor roofcol = TCODColor::darkOrange;
  x_ = dungeonDoorx - doorx;
  y_ = dungeonDoory - doory;
  for (int cy = 0; cy < h_; cy++) {
    for (int cx = 0; cx < w_; cx++) {
      if (!IN_RECTANGLE(x_ + cx, y_ + cy, dungeon->width, dungeon->height)) continue;
      int cellType = map[cx + cy * w_];
      switch (cellType) {
        case BUILDING_NONE:
          break;
        case BUILDING_WALL_N:
        case BUILDING_WALL_E:
        case BUILDING_WALL_S:
        case BUILDING_WALL_W:
        case BUILDING_WALL_NW:
        case BUILDING_WALL_NE:
        case BUILDING_WALL_SE:
        case BUILDING_WALL_SW: {
          // walls
          static int wallToChar[] = {
              0,
              0,
              TCOD_CHAR_HLINE,
              TCOD_CHAR_VLINE,
              TCOD_CHAR_HLINE,
              TCOD_CHAR_VLINE,
              TCOD_CHAR_NW,
              TCOD_CHAR_NE,
              TCOD_CHAR_SE,
              TCOD_CHAR_SW};
          item::Item* wall = item::Item::getItem(cityWalls ? "city wall" : "wall", x_ + cx, y_ + cy);
          wall->ch_ = wallToChar[cellType];
          dungeon->addItem(wall);
        }
        // no break!
        // concerns also walls
        case BUILDING_FLOOR:
        case BUILDING_WINDOW_H:
        case BUILDING_WINDOW_V:
        case BUILDING_DOOR:
          // floor
          map::Cell* cell = dungeon->getCell(x_ + cx, y_ + cy);
          cell->terrain = TERRAIN_WOODEN_FLOOR;
          cell->building = this;
          static int subcx[] = {0, 1, 0, 1};
          static int subcy[] = {0, 0, 1, 1};
          int dungeon2x = (int)(x_ + cx) * 2;
          int dungeon2y = (int)(y_ + cy) * 2;
          // subcell stuff
          for (int i = 0; i < 4; i++) {
            int d2x = dungeon2x + subcx[i];
            int d2y = dungeon2y + subcy[i];
            dungeon->setGroundColor(d2x, d2y, TCODColor::darkerAmber);
            dungeon->setShadowHeight(d2x, d2y, 1.0f);
            // roof
            dungeon->canopy->putPixel(d2x, d2y, cx * 2 + subcx[i] < w_ ? roofcol * 0.7f : roofcol);
          }
          if (cellType == BUILDING_DOOR) {
            dungeon->addItem(item::Item::getItem("door", x_ + cx, y_ + cy));
          } else if (cellType == BUILDING_WINDOW_H) {
            item::Item* item = item::Item::getItem(cityWalls ? "arrow slit" : "window", x_ + cx, y_ + cy);
            item->ch_ = TCOD_CHAR_HLINE;
            dungeon->addItem(item);
          } else if (cellType == BUILDING_WINDOW_V) {
            item::Item* item = item::Item::getItem(cityWalls ? "arrow slit" : "window", x_ + cx, y_ + cy);
            item->ch_ = TCOD_CHAR_VLINE;
            dungeon->addItem(item);
          }
          break;
      }
    }
  }
}

// break roof too far away from a wall
void Building::collapseRoof() {
  item::ItemType* wall = item::Item::getType("wall");
  map::Dungeon* dungeon = gameEngine->dungeon;
  for (int cy = 0; cy < h_; cy++) {
    for (int cx = 0; cx < w_; cx++) {
      if (!IN_RECTANGLE(x_ + cx, y_ + cy, dungeon->width, dungeon->height)) continue;
      int cellType = map[cx + cy * w_];
      if (cellType >= BUILDING_WALL_N && cellType <= BUILDING_WALL_SW) {
        if (dungeon->getItem((int)(x_ + cx), (int)(y_ + cy), wall)) return;  // there is still a wall!
      }
    }
  }
  for (int cy = 0; cy < h_; cy++) {
    for (int cx = 0; cx < w_; cx++) {
      if (!IN_RECTANGLE(x_ + cx, y_ + cy, dungeon->width, dungeon->height)) continue;
      int cellType = map[cx + cy * w_];
      if (cellType != BUILDING_NONE) {
        int dx = (int)((x_ + cx) * 2);
        int dy = (int)((y_ + cy) * 2);
        dungeon->canopy->putPixel(dx, dy, TCODColor::black);
        dungeon->canopy->putPixel(dx + 1, dy, TCODColor::black);
        dungeon->canopy->putPixel(dx, dy + 1, TCODColor::black);
        dungeon->canopy->putPixel(dx + 1, dy + 1, TCODColor::black);
      }
    }
  }
}

void Building::setBuildingWallCell(int x, int y, int ysym, int ch, map::Dungeon* dungeon) {
  static int subcx[] = {0, 1, 0, 1};
  static int subcy[] = {0, 0, 1, 1};
  item::Item* wall = item::Item::getItem("city wall", x, y);
  wall->ch_ = ch;
  dungeon->addItem(wall);
  dungeon->getCell(x, y)->terrain = TERRAIN_WOODEN_FLOOR;
  for (int i = 0; i < 4; i++) {
    int d2x = x * 2 + subcx[i];
    int d2y = y * 2 + subcy[i];
    dungeon->setGroundColor(d2x, d2y, TCODColor::darkerAmber);
    dungeon->setShadowHeight(d2x, d2y, 3.0f);
  }
  int y2 = 2 * ysym - y;
  if (y2 != y && y2 >= 0 && y2 < dungeon->height) {
    wall = item::Item::getItem("city wall", x, y2);
    switch (ch) {
      case TCOD_CHAR_SW:
        ch = TCOD_CHAR_NW;
        break;
      case TCOD_CHAR_SE:
        ch = TCOD_CHAR_NE;
        break;
      case TCOD_CHAR_NW:
        ch = TCOD_CHAR_SW;
        break;
      case TCOD_CHAR_NE:
        ch = TCOD_CHAR_SE;
        break;
      default:
        break;
    }
    wall->ch_ = ch;
    dungeon->addItem(wall);
    dungeon->getCell(x, y2)->terrain = TERRAIN_WOODEN_FLOOR;
    for (int i = 0; i < 4; i++) {
      int d2x = x * 2 + subcx[i];
      int d2y = y2 * 2 + subcy[i];
      dungeon->setGroundColor(d2x, d2y, TCODColor::darkerAmber);
      dungeon->setShadowHeight(d2x, d2y, 3.0f);
    }
  }
}

void Building::buildCityWalls(int x, map::Dungeon* dungeon) {
  int ysym = TCODRandom::getInstance()->getInt(20, dungeon->height - 20);
  int miny = 0;
  int maxy = ysym;
  if (ysym < dungeon->height / 2) {
    miny = ysym;
    maxy = dungeon->height - 1;
  }
  for (int y = miny; y <= maxy; y++) {
    int rnd = TCODRandom::getInstance()->getInt(0, 100);
    if (rnd < 8) {
      int dir = (rnd < 4 ? -1 : 1);
      setBuildingWallCell(x, y, ysym, dir == 1 ? TCOD_CHAR_SW : TCOD_CHAR_SE, dungeon);
      int l = TCODRandom::getInstance()->getInt(1, 2);
      x += dir;
      for (; l > 0; x += dir, l--) {
        setBuildingWallCell(x, y, ysym, TCOD_CHAR_HLINE, dungeon);
      }
      setBuildingWallCell(x, y, ysym, dir == 1 ? TCOD_CHAR_NE : TCOD_CHAR_NW, dungeon);
    } else if (rnd < 30) {
      Building* tower = Building::generateWallsOnly(3, 3, 1, NULL);
      tower->map[2 + 1 * 3] = BUILDING_DOOR;
      tower->doorx = 2;
      tower->doory = 1;
      tower->map[1 * 3] = BUILDING_WINDOW_V;
      tower->applyTo(dungeon, x + 1, y, true);

      mob::Creature* archer = mob::Creature::getCreature(mob::CREATURE_ARCHER);
      archer->setPos(x, y);
      dungeon->addCreature(archer);

      tower = Building::generateWallsOnly(3, 3, 1, NULL);
      tower->map[2 + 1 * 3] = BUILDING_DOOR;
      tower->map[1 * 3] = BUILDING_WINDOW_V;
      tower->doorx = 2;
      tower->doory = 1;
      tower->applyTo(dungeon, x + 1, 2 * ysym - y, true);
      if ((unsigned)(2 * ysym - y) < (unsigned)dungeon->height) {
        archer = mob::Creature::getCreature(mob::CREATURE_ARCHER);
        archer->setPos(x, 2 * ysym - y);
        dungeon->addCreature(archer);
      }
      y++;
    } else
      setBuildingWallCell(x, y, ysym, TCOD_CHAR_VLINE, dungeon);
  }
}
}  // namespace map
