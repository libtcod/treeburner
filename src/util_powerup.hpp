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
#include <libtcod.hpp>

typedef enum { PW_FIRST = 0, PW_FB = 0, PW_BURST, PW_SPARKLE_THROUGH, PW_SPARKLE_BOUNCE, PW_INCAN, PW_NB } PowerupId;

class Powerup {
 public:
  PowerupId id;

  const char* name = nullptr;
  int level;
  const char* type = nullptr;
  const char* description = nullptr;
  Powerup*
    requires
  = nullptr;
  bool enabled;

  static TCODList<Powerup*> list;

  static void init();
  Powerup(
      PowerupId id,
      int level,
      const char* name = NULL,
      const char* type = NULL,
      const char* description = NULL,
      Powerup* requires = NULL);
  void apply();
  static void getAvailable(TCODList<Powerup*>* l);
};

class PowerupGraph {
 public:
  static PowerupGraph* instance;
  void refresh() { needRefresh = true; }
  void setFontSize(int fontSize);
  void render();
  bool update(float elapsed, TCOD_key_t key, TCOD_mouse_t* mouse);

 protected:
  static TCODImage* img[PW_NB];
  static const char* imgName[PW_NB];
  bool needRefresh;
  int iconWidth, iconHeight;  // in console cells
  int mousecx, mousecy;
  Powerup* selected = nullptr;

  friend class Powerup;
  PowerupGraph();
};
