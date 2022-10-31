/*
 * Copyright (c) 2010 Jice
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
#include "mob_fish.hpp"

#include <assert.h>
#include <math.h>

#include "main.hpp"

Fish::Fish(WaterZone* zone) : Creature(), zone(zone) {
  strcpy(name, "fish");
  ch = 0;
  color_ = TCODColor::desaturatedSky;
  life = 10;
  speed = 12.0f;
  type = CREATURE_FISH;
  flags = CREATURE_NOTBLOCK | CREATURE_CATCHABLE;
  height = 0.5f;
  dx = TCODRandom::getInstance()->getFloat(-2.0f, 2.0f);
  dy = TCODRandom::getInstance()->getFloat(-2.0f, 2.0f);
  oldx = oldy = -1.0f;
  updated = false;
}

Fish::~Fish() {
  if (zone) zone->shoal->list.removeFast(this);
}

void Fish::render(LightMap& lightMap) {
  // position on console
  int conx = (int)(x - gameEngine->xOffset);
  int cony = (int)(y - gameEngine->yOffset);
  if (!IN_RECTANGLE(conx, cony, CON_W, CON_H)) return;  // out of console

  float playerDist = distance(gameEngine->player);
  float apparentHeight = height / playerDist;
  if (apparentHeight < MIN_VISIBLE_HEIGHT) return;  // too small to see at that distance
  apparentHeight -= MIN_VISIBLE_HEIGHT;
  float coef = 0.5f * apparentHeight / VISIBLE_HEIGHT;
  coef = MIN(0.5f, coef);

  int conx2 = getSubX() - gameEngine->xOffset * 2;
  int cony2 = getSubY() - gameEngine->yOffset * 2;
  TCODColor rcol = color_;
  TCODColor bgcol = gameEngine->ground.getPixel(conx2, cony2);
  rcol = TCODColor::lerp(bgcol, rcol, coef);
  gameEngine->ground.putPixel(conx2, cony2, rcol);
}

void Fish::initItem() {
  asItem = item::Item::getItem("living fish", x, y);
  asItem->asCreature = this;
}

bool Fish::wasOnScreen() const {
  return IN_RECTANGLE(oldx - gameEngine->xOffset, oldy - gameEngine->yOffset, CON_W, CON_H);
}

// move the fish but keep it in water, sliding against the lake coasts
void Fish::slide() {
  // old and new subcell coordinates
  int oldx2 = (int)(oldx * 2);
  int oldy2 = (int)(oldy * 2);
  int newx2 = getSubX();
  int newy2 = getSubY();
  if (newx2 != oldx2 || newy2 != oldy2) {
    // move the fish while keeping it in water
    Dungeon* dungeon = gameEngine->dungeon;
    if (!IN_RECTANGLE(newx2, newy2, dungeon->width * 2, dungeon->height * 2)) {
      x = oldx;
      y = oldy;
    } else if (!dungeon->hasWater(newx2, newy2)) {
      int dx = newx2 - oldx2;
      int dy = newy2 - oldy2;
      if (dx && dy) {
        // diagonal move
        if (dungeon->hasWater(newx2, oldy2)) {
          y = oldy;
          dy = -dy;
        } else if (dungeon->hasWater(oldx2, newy2)) {
          x = oldx;
          dx = -dx;
        } else {
          x = oldx;
          y = oldy;
          dx = -dx;
          dy = -dy;
        }
      } else if (dx) {
        // horizontal move
        if (dungeon->hasWater(newx2, newy2 + 1)) {
          y = y + 0.5f;
        } else if (dungeon->hasWater(newx2, newy2 - 1)) {
          y = y - 0.5f;
        } else {
          x = oldx;
          y = oldy;
          dx = -dx;
          dy = -dy;
        }
      } else if (dy) {
        // vertical move
        if (dungeon->hasWater(newx2 + 1, newy2)) {
          x = x + 0.5f;
        } else if (dungeon->hasWater(newx2 - 1, newy2)) {
          x = x - 0.5f;
        } else {
          x = oldx;
          y = oldy;
          dx = -dx;
          dy = -dy;
        }
      }
    }
    if ((int)x != (int)oldx || (int)y != (int)oldy) dungeon->moveCreature(this, (int)oldx, (int)oldy, (int)x, (int)y);
  }
  updated = false;
}

bool Fish::update(float elapsed) {
  bool ret = Creature::update(elapsed);
  if (!ret) return false;
  if (gameEngine->isGamePaused()) return true;
  oldx = x;
  oldy = y;
  assert(gameEngine->dungeon->hasWater(getSubX(), getSubY()));
  x += dx * elapsed;
  y += dy * elapsed;
  // printf ("%x %d %d -> %d %d\n",this,(int)oldx,(int)oldy,(int)x,(int)y);

  dx += elapsed * TCODRandom::getInstance()->getFloat(-20.0f, 20.0f);
  dy += elapsed * TCODRandom::getInstance()->getFloat(-20.0f, 20.0f);
  if (ABS(dx) > 2.0f) dx = dx * (1.0f - elapsed);
  if (ABS(dy) > 2.0f) dy = dy * (1.0f - elapsed);
  updated = true;
  return true;
}
