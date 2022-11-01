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

#include "base/gameengine.hpp"
#include "base/savegame.hpp"
#include "mob/friend.hpp"
#include "ui_input.hpp"

namespace screen {
class ForestScreen : public base::GameEngine, public base::SaveListener {
 public:
  mob::Friend* fr;

  ForestScreen();

  void render() override;
  bool update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) override;
  void onEvent(const SDL_Event&) override{};
  void generateMap(uint32_t seed);  // generate a new random map
  void loadMap(uint32_t seed);  // load map from savegame

  void onFontChange();

  // SaveListener
  bool loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) override;
  void saveData(uint32_t chunkId, TCODZip* zip) override;

 protected:
  TCODRandom* forestRng;

  void onActivate() override;
  void onDeactivate() override;
  void placeTree(map::Dungeon* dungeon, int x, int y, const item::ItemType* treeType);
  void placeHouse(map::Dungeon* dungeon, int doorx, int doory, base::Entity::Direction dir);
  int debugMap;
  TextInput textInput;
};
}  // namespace screen
