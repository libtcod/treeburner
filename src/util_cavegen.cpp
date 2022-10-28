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
#include <stdio.h>

#include "main.hpp"

// allocate all data
void CaveGenerator::initData(int size) {
  this->size = size;
  size2x = size * 2;
  map = new TCODMap(size, size);
  map2x = new TCODMap(size2x, size2x);
  ground = new TCODImage(size2x, size2x);
}

CaveGenerator::CaveGenerator(int level) : level(level) {
  // get dungeons min/max size from config
  static int nbLevels = config.getIntProperty("config.gameplay.nbLevels");
  static int minSize = config.getIntProperty("config.gameplay.dungeonMinSize");
  static int maxSize = config.getIntProperty("config.gameplay.dungeonMaxSize");

  initData(minSize + (maxSize - minSize) * (level + 1) / nbLevels);

  // prepare dungeon generation variables
  TCODBsp bsp(0, 0, size, size);

  bspDepth = 2;
  int tmpSize = size;
  minRoomSize = 6;
  while (tmpSize > minRoomSize) {
    tmpSize /= 2;
    bspDepth++;
  }
  randomRoom = level > 0;
  roomWalls = level < nbLevels / 3;

  // bsp map generation
  bsp.splitRecursive(rng, bspDepth, minRoomSize + (roomWalls ? 1 : 0), minRoomSize + (roomWalls ? 1 : 0), 1.5f, 1.5f);
  bsp.traverseInvertedLevelOrder(this, NULL);

  if (level == 2) {
    // bsp with rounded corners
    CellularAutomata cell(map);
    cell.generate(&CellularAutomata::CAFunc_roundCorners, 1);
    cell.seal();
    cell.connect();
    cell.apply(map);
  } else if (level == 3) {
    // slightly cavified bsp
    CellularAutomata cell(map);
    cell.generate(&CellularAutomata::CAFunc_cave2, 1);
    cell.generate(&CellularAutomata::CAFunc_dig, 1);
    cell.generate(&CellularAutomata::CAFunc_cave, 1);
    cell.generate(&CellularAutomata::CAFunc_dig, 1);
    cell.seal();
    cell.connect();
    cell.apply(map);
  } else if (level > nbLevels / 3) {
    // cavification
    // the clean dungeon
    CellularAutomata cell(map);

    // the cavified dungeon
    CellularAutomata* cavecell = NULL;
    if (level < 2 * nbLevels / 3)
      cavecell = new CellularAutomata(&cell);
    else
      cavecell = new CellularAutomata(size, size, 40);
    // cavecell.generate(&CellularAutomata::CAFunc_dig,1);
    cavecell->generate(&CellularAutomata::CAFunc_cave, 4);
    cavecell->generate(&CellularAutomata::CAFunc_cave2, 3);
    cavecell->connect();

    // the morphed dungeon
    CellularAutomata morph(cavecell, &cell, ((float)(level - nbLevels / 3) / (nbLevels - nbLevels / 3)));
    morph.generate(&CellularAutomata::CAFunc_cave, 4);
    morph.generate(&CellularAutomata::CAFunc_dig, 1);
    morph.seal();
    morph.connect();
    morph.apply(map);
    delete cavecell;
  }
}

// simple 2x2 blurring kernel
void CaveGenerator::smoothImage(TCODImage* img) {
  int width, height;
  img->getSize(&width, &height);
  for (int x = 0; x < width - 1; x++) {
    for (int y = 0; y < height - 1; y++) {
      int r = 0, g = 0, b = 0;
      TCODColor col = img->getPixel(x, y);
      r += col.r;
      g += col.g;
      b += col.b;
      col = img->getPixel(x + 1, y);
      r += col.r;
      g += col.g;
      b += col.b;
      col = img->getPixel(x + 1, y + 1);
      r += col.r;
      g += col.g;
      b += col.b;
      col = img->getPixel(x, y + 1);
      r += col.r;
      g += col.g;
      b += col.b;
      img->putPixel(x, y, TCODColor(r / 4, g / 4, b / 4));
    }
  }
}

// bsp level generation code

// the class building the dungeon from the bsp nodes
bool CaveGenerator::visitNode(TCODBsp* node, void* userData) {
  if (node->isLeaf()) {
    // calculate the room size
    int minx = node->x + 1;
    int maxx = node->x + node->w - 1;
    int miny = node->y + 1;
    int maxy = node->y + node->h - 1;
    if (!roomWalls) {
      if (minx > 1) minx--;
      if (miny > 1) miny--;
    }
    if (maxx == map->getWidth() - 1) maxx--;
    if (maxy == map->getHeight() - 1) maxy--;
    if (randomRoom) {
      minx = rng->getInt(minx, maxx - minRoomSize + 1);
      miny = rng->getInt(miny, maxy - minRoomSize + 1);
      maxx = rng->getInt(minx + minRoomSize - 1, maxx);
      maxy = rng->getInt(miny + minRoomSize - 1, maxy);
    }
    // always walls on map borders
    minx = MAX(1, minx);
    miny = MAX(1, miny);
    maxx = MIN(size - 2, maxx);
    maxy = MIN(size - 2, maxy);
    // resize the node to fit the room
    node->x = minx;
    node->y = miny;
    node->w = maxx - minx + 1;
    node->h = maxy - miny + 1;
    // dig the room
    MapCarver::room(map, node->x, node->y, node->w, node->h);
  } else {
    // resize the node to fit its sons
    TCODBsp* left = node->getLeft();
    TCODBsp* right = node->getRight();
    node->x = MIN(left->x, right->x);
    node->y = MIN(left->y, right->y);
    node->w = MAX(left->x + left->w, right->x + right->w) - node->x;
    node->h = MAX(left->y + left->h, right->y + right->h) - node->y;
    // create a corridor between the two lower nodes
    if (node->horizontal) {
      // vertical corridor
      if (left->x + left->w - 1 < right->x || right->x + right->w - 1 < left->x) {
        // no overlapping zone. we need a Z shaped corridor
        int x1 = rng->getInt(left->x, left->x + left->w - 1);
        int x2 = rng->getInt(right->x, right->x + right->w - 1);
        int y = rng->getInt(left->y + left->h, right->y);
        MapCarver::vlineUp(map, x1, y - 1);
        MapCarver::hline(map, x1, y, x2);
        MapCarver::vlineDown(map, x2, y + 1);
      } else {
        // straight vertical corridor
        int minx = MAX(left->x, right->x);
        int maxx = MIN(left->x + left->w - 1, right->x + right->w - 1);
        int x = rng->getInt(minx, maxx);
        MapCarver::vlineDown(map, x, right->y);
        MapCarver::vlineUp(map, x, right->y - 1);
      }
    } else {
      // horizontal corridor
      if (left->y + left->h - 1 < right->y || right->y + right->h - 1 < left->y) {
        // no overlapping zone. we need a Z shaped corridor
        int y1 = rng->getInt(left->y, left->y + left->h - 1);
        int y2 = rng->getInt(right->y, right->y + right->h - 1);
        int x = rng->getInt(left->x + left->w, right->x);
        MapCarver::hlineLeft(map, x - 1, y1);
        MapCarver::vline(map, x, y1, y2);
        MapCarver::hlineRight(map, x + 1, y2);
      } else {
        // straight horizontal corridor
        int miny = MAX(left->y, right->y);
        int maxy = MIN(left->y + left->h - 1, right->y + right->h - 1);
        int y = rng->getInt(miny, maxy);
        MapCarver::hlineLeft(map, right->x - 1, y);
        MapCarver::hlineRight(map, right->x, y);
      }
    }
  }
  return true;
}
