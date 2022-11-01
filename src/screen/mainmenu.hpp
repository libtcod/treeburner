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

#include "screen.hpp"
#include "util_fire.hpp"

namespace screen {
enum MenuItemId { MENU_NEW_GAME, MENU_CONTINUE, MENU_EXIT, MENU_NB_ITEMS };

class MainMenu : public Screen {
 public:
  MainMenu();
  static MainMenu* instance;
  void render() override;
  void onEvent(const SDL_Event&) override{};
  bool update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) override;
  void waitForWorldGen();
  void waitForForestGen();

 protected:
  void onInitialise() override;
  void onActivate() override;
  void computeSmoke(float z, TCODImage* img, int miny, int maxy);
  TCODList<MenuItemId> menu;
  int selectedItem;
  float elapsed;
  float smokeElapsed;
  float noiseZ;
  TCODImage* img;
  int worldGenJobId;
  int forestGenJobId;
  // for background world generation
  TCOD_thread_t backgroundThread;
  TCOD_semaphore_t worldDone;
  // title position & size
  int titlex, titley, titlew, titleh;
  Fire* fire;
};
}  // namespace screen
