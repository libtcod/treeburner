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
#include "util/fire.hpp"

#include <stdlib.h>
#include <string.h>

#include "constants.hpp"
#include "main.hpp"

// #define FIRE_DEBUG

// old school fire routines

TCODColor fireColor[256];
static bool col_init = false;

#define UPDATE_DELAY 0.05f
#define FIRE_SET(x, y, v) buf[(y) * (w + 2) + (x)] = (v)
#define FIRE_SET2(x, y, v) smoothedBuf[(y) * (w + 2) + (x)] = (v)
#define FIRE_GET(x, y) buf[(y) * (w + 2) + (x)]
#define FIRE_GET2(x, y) smoothedBuf[(y) * (w + 2) + (x)]

Fire::Fire(int w, int h) : w(w), h(h), el(0.0f) {
  if (!col_init) {
    int i;
    for (i = 0; i < 128; i++) {
      fireColor[i].r = MIN(i * 3, 255);
      fireColor[i].g = i * 2;
      fireColor[i].b = i / 2;
      fireColor[i + 128].r = 255;
      fireColor[i + 128].g = 255;
      fireColor[i + 128].b = MIN(64 + 192 * i / 128, 255);
    }
    col_init = true;
  }
  img = new TCODImage(w, h);
  buf = new uint8_t[(w + 2) * h];
  smoothedBuf = new uint8_t[(w + 2) * h];
  memset(buf, 0, sizeof(uint8_t) * (w + 2) * h);
  memset(smoothedBuf, 0, sizeof(uint8_t) * (w + 2) * h);
}

void Fire::generateImage() {
  for (int x = 1; x <= w; x++) {
    for (int y = 0; y < h; y++) {
      img->putPixel(x - 1, y, fireColor[FIRE_GET2(x, y)]);
    }
  }
}

void Fire::spark(int x, int y) { softspark(x, y, 48); }

void Fire::antispark(int x, int y) { softspark(x, y, -48); }

void Fire::softspark(int x, int y, int delta) {
  int v = (int)(FIRE_GET(x, y)) + delta;
  v = CLAMP(0, 255, v);
  FIRE_SET(x, y, (uint8_t)v);
}

void Fire::update(float elapsed) {
  el += elapsed;
  if (el < UPDATE_DELAY) return;
  el = 0.0f;
  /*
  int x,y;
  int off1 = (h-1)*(w+2);
  int off2 = (h-2)*(w+2);
  for (x=0; x < w+2; x++) {
          uint8_t v = TCODRandom::getInstance()->getInt(0,255);
          buf[ off1 + x ] = v;
          buf[ off2 + x ] = MAX(v-4,0);
  }
  */
  for (int y = 1; y < h - 1; y++) {
    for (int x = 1; x <= w; x++) {
      int x2 = x + TCODRandom::getInstance()->getInt(-1, 1);
      int v = (int)(FIRE_GET(x, y)) * 4 + (int)(FIRE_GET(x, y + 1)) * 4 + (int)(FIRE_GET(x + 1, y + 1)) +
              (int)(FIRE_GET(x - 1, y + 1));
      v /= 10;
      v -= 4;
      if (y < 10) v -= 4;
      if (v < 0) v = 0;
      FIRE_SET(x2, y - 1, (uint8_t)v);
    }
  }

  for (int y = 0; y < h - 1; y++) {
    for (int x = 1; x <= w; x++) {
      int v =
          (int)(FIRE_GET(x, y)) + (int)(FIRE_GET(x, y + 1)) + (int)(FIRE_GET(x + 1, y + 1)) + (int)(FIRE_GET(x + 1, y));
      v /= 4;
      FIRE_SET2(x, y, (uint8_t)v);
    }
  }
}

Fire::~Fire() {
  delete buf;
  delete smoothedBuf;
  delete img;
}

FireManager::FireManager(map::Dungeon* dungeon) : dungeon(dungeon), el(0.0f) {
  buf = new uint8_t[dungeon->width * dungeon->height * 4];
  memset(buf, 0, sizeof(uint8_t) * dungeon->width * dungeon->height * 4);
  if (!col_init) {
    int i;
    for (i = 0; i < 128; i++) {
      fireColor[i].r = MIN(i * 3, 255);
      fireColor[i].g = i * 2;
      fireColor[i].b = i / 2;
      fireColor[i + 128].r = 255;
      fireColor[i + 128].g = 255;
      fireColor[i + 128].b = MIN(64 + 192 * i / 128, 255);
    }
    col_init = true;
  }
}

FireManager::~FireManager() { delete buf; }

void FireManager::spark(int x, int y) { softspark(x, y, 48); }

void FireManager::antispark(int x, int y) { softspark(x, y, -48); }

void FireManager::softspark(int x, int y, int delta) {
  int v = (int)(get(x, y)) + delta;
  v = CLAMP(0, 255, v);
  set(x, y, (uint8_t)v);
}

void FireManager::addZone(int x, int y, int w, int h) {
  FireZone z;
  x = MAX(0, x);
  y = MAX(0, y);
  w = MIN(dungeon->width * 2 - 1 - x, w);
  h = MIN(dungeon->height * 2 - 1 - y, h);
  z.r = base::Rect(x, y, w, h);
  z.life = -1.0f;
  zones.push(z);
#ifdef FIRE_DEBUG
  gameEngine->gui.log.debug("FireManager(%d)::addZone %d %d %d %d", zones.size(), x, y, w, h);
#endif
}

void FireManager::removeZone(int x, int y, int w, int h) {
  static float zoneDecay = config.getFloatProperty("config.fireManager.zoneDecay");
  for (FireZone* z = zones.begin(); z != zones.end(); z++) {
    if (z->r.x == x && z->r.y == y && z->r.w == w && z->r.h == h) {
      z->life = zoneDecay;
      break;
    }
  }
#ifdef FIRE_DEBUG
  gameEngine->gui.log.debug("FireManager(%d)::removeZone %d %d %d %d", zones.size(), x, y, w, h);
#endif
}

void FireManager::update(float elapsed) {
  static float zoneDecay = config.getFloatProperty("config.fireManager.zoneDecay");
  el += elapsed;
  if (el < UPDATE_DELAY) return;
  el = 0.0f;
  base::Rect screenZone;
  screenZone.x = gameEngine->xOffset * 2;
  screenZone.w = CON_W * 2;
  screenZone.y = gameEngine->yOffset * 2;
  screenZone.h = CON_H * 2;
  screenFireZone.x = screenZone.x + screenZone.w;
  screenFireZone.y = screenZone.y + screenZone.h;
  screenFireZone.w = (int)screenZone.x - 1;
  screenFireZone.h = (int)screenZone.y - 1;
  for (FireZone* z = zones.begin(); z != zones.end(); z++) {
    if (z->life > 0.0f) {
      z->life -= elapsed;
      if (z->life < 0.0f) {
        z = zones.removeFast(z);
        continue;
      }
    }
    if (z->r.isIntersecting(screenZone)) {
      if (z->r.x < screenFireZone.x) {
        screenFireZone.x = z->r.x;
      }
      if (z->r.x + z->r.w > screenFireZone.w) {
        screenFireZone.w = (int)(z->r.x + z->r.w);
      }
      if (z->r.y < screenFireZone.y) {
        screenFireZone.y = z->r.y;
      }
      if (z->r.y + z->r.h > screenFireZone.h) {
        screenFireZone.h = (int)(z->r.y + z->r.h);
      }
      int prob = 48;
      if (z->life > 0.0f) {
        prob += (int)(32 * (zoneDecay - z->life) / zoneDecay);
      }
      // trigger new sparks
      for (int x = (int)(z->r.x + 1); x < (int)(z->r.x + z->r.w - 1); x++) {
        for (int y = (int)(z->r.y + z->r.h - 3); y < (int)(z->r.y + z->r.h - 1); y++) {
          int v = TCODRandom::getInstance()->getInt(24, 64);
          // v += buf[x+y*dungeon->width*2];
          // v = MIN(255,v);
          if (v >= prob) buf[x + y * dungeon->width * 2] = v;
        }
      }
    }
  }
  // update fire
  if (screenFireZone.w > screenFireZone.x) {
    screenFireZone.x = MAX(1, screenFireZone.x);
    screenFireZone.y = MAX(1, screenFireZone.y);
    screenFireZone.w = MIN(dungeon->width * 2 - 2, screenFireZone.w);
    screenFireZone.h = MIN(dungeon->height * 2 - 2, screenFireZone.h);
    for (int x = (int)screenFireZone.x; x < screenFireZone.w; x++) {
      for (int y = (int)screenFireZone.y; y < screenFireZone.h; y++) {
        int x2 = x + TCODRandom::getInstance()->getInt(-1, 1);
        int v = (int)(get(x, y)) * 4 + (int)(get(x, y + 1)) * 4 + (int)(get(x + 1, y + 1)) + (int)(get(x - 1, y + 1));
        v /= 10;
        v -= 4;
        if (v < 0) v = 0;
        set(x2, y - 1, (uint8_t)v);
        if (v > 24 && TCODRandom::getInstance()->getInt(0, 100) < 5) {
          // burn ground
          TCODColor col = dungeon->getGroundColor(x2, y - 1);
          col = col * 0.98f;
          dungeon->setGroundColor(x2, y - 1, col);
        }
      }
    }
  }
  /*
  int x,y;
  int off1 = (h-1)*(w+2);
  int off2 = (h-2)*(w+2);
  for (x=0; x < w+2; x++) {
          uint8_t v = TCODRandom::getInstance()->getInt(0,255);
          buf[ off1 + x ] = v;
          buf[ off2 + x ] = MAX(v-4,0);
  }
  */
}

void FireManager::renderFire(TCODImage& ground) {
  int dx = gameEngine->xOffset * 2;
  int dy = gameEngine->yOffset * 2;
  screenFireZone.x = MAX(dx, screenFireZone.x);
  screenFireZone.y = MAX(dy, screenFireZone.y);
  screenFireZone.w = MIN(dungeon->width * 2 + dx, screenFireZone.w);
  screenFireZone.h = MIN(dungeon->height * 2 + dy, screenFireZone.h);
  for (int x = (int)screenFireZone.x; x < screenFireZone.w; x++) {
    for (int y = (int)screenFireZone.y; y < screenFireZone.h; y++) {
      uint8_t v = get(x, y);
      if (v > 0) {
        map::HDRColor col = fireColor[v];
        col = col * 1.5f + ground.getPixel(x - dx, y - dy);
        ground.putPixel(x - dx, y - dy, col);
      }
    }
  }
}
