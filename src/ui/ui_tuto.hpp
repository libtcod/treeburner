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

#include "base/savegame.hpp"
#include "screen.hpp"

typedef enum {
  TUTO_NONE,
  TUTO_FOOD,
  TUTO_MOVE,
  TUTO_RUN,
  TUTO_CROUCH,
  TUTO_HIDE_SEEK,
  TUTO_ITEMS,
  TUTO_INVENTORY,
  TUTO_INVENTORY2,
  TUTO_FIREBALL,
  TUTO_NB_PAGES
} TutorialPageId;

struct TutorialPage {
  const char* name = nullptr;
  TCODConsole* con = nullptr;
  bool inMenu;
  int x, y;
  float delay;
};

class Tutorial : public Screen, public base::SaveListener {
 public:
  Tutorial();
  virtual ~Tutorial();
  void render() override;
  void onEvent(const SDL_Event&) override{};
  bool update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) override;
  void startLiveTuto(TutorialPageId id);
  void closeLiveTuto();
  void openMenu();
  void closeMenu();
  void enableMenuPage(TutorialPageId id);
  void disableMenuPage(TutorialPageId id);
  void init();

  // SaveListener
  bool loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) override;
  void saveData(uint32_t chunkId, TCODZip* zip) override;

 protected:
  void onInitialise() override;
  void startTuto(TutorialPageId id);
  float blinkDelay;
  TutorialPageId id;
  float tutoElapsed;
  float fadeOutDelay;
  int x, y, w, h;
  bool renderMenu;
  int selectedItem;
  TutorialPageId lastPage;
  TutorialPage pages[TUTO_NB_PAGES];
  TCODList<TutorialPageId> toStart;
  TCODList<TutorialPageId> menu;
  void setNewPage(TutorialPageId newId);
  bool alreadyStarted[TUTO_NB_PAGES];
  void createPage(TutorialPageId id);
  TCODConsole* createMovePage();
  TCODConsole* createSprintPage();
  TCODConsole* createFoodPage();
  TCODConsole* createHideSeekPage();
  TCODConsole* createItemsPage();
  TCODConsole* createInventoryPage();
  TCODConsole* createInventory2Page();
  TCODConsole* createCrouchPage();
  TCODConsole* createFireballPage();
  TCODConsole* createBasePage(const char* name, int w, int h);
  TCODConsole* createPage(TutorialPageId id, const char* title, const char* content, ...);
};
