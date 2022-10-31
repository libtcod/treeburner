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
#pragma once
#include <libtcod.hpp>

#include "map_dungeon.hpp"

extern TCODColor fireColor[256];

class Fire {
 public:
  Fire(int w, int h);
  ~Fire();
  void generateImage();
  void spark(int x, int y);
  void antispark(int x, int y);
  void softspark(int x, int y, int delta);
  void update(float elapsed);
  TCODImage* img = nullptr;

 protected:
  int w, h;
  uint8_t* buf = nullptr;
  uint8_t* smoothedBuf = nullptr;
  float el;
};

class FireManager {
 public:
  FireManager(Dungeon* dungeon);
  ~FireManager();
  void spark(int x, int y);
  void antispark(int x, int y);
  void softspark(int x, int y, int delta);
  void update(float elapsed);
  void renderFire(TCODImage& ground);
  void addZone(int x, int y, int w, int h);
  void removeZone(int x, int y, int w, int h);

 protected:
  inline uint8_t get(int x, int y) { return buf[x + dungeon->width * 2 * y]; }
  inline void set(int x, int y, uint8_t v) { buf[x + dungeon->width * 2 * y] = v; }
  struct FireZone {
    base::Rect r;
    float life;
  };
  TCODList<FireZone> zones;
  Dungeon* dungeon = nullptr;
  uint8_t* buf = nullptr;
  float el;
  base::Rect screenFireZone;
};
