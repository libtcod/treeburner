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
#include "bas_gameengine.hpp"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "bas_entity.hpp"
#include "item.hpp"
#include "main.hpp"
#include "spell_fireball.hpp"

GameEngine* gameEngine = nullptr;

GameEngine::GameEngine() : Screen{0} { gameEngine = this; }

void GameEngine::onActivate() {
  Screen::onActivate();
  hitFlashAmount = 0.0f;
  firstFrame = true;
  computeAspectRatio();
  gui.activate();
  memset(&stats, 0, sizeof(stats));
  TCODConsole::mapAsciiCodeToFont(TCOD_CHAR_PROGRESSBAR, 26, 3);
  isUpdatingFireballs = false;
}

void GameEngine::onDeactivate() {
  Screen::onDeactivate();
  gui.deactivate();
}

void GameEngine::onFontChange() { computeAspectRatio(); }

void GameEngine::recomputeCanopy(item::Item* it) {
  static const int treeRadius = config.getIntProperty("config.display.treeRadius");
  if (dungeon->canopy) {
    if (it) {
      // reset only for one tree
      Rect r(it->x * 2 - treeRadius - 1, it->y * 2 - treeRadius - 1, treeRadius * 2 + 2, treeRadius * 2 + 2);
      r.x = MAX(0, r.x);
      r.y = MAX(0, r.y);
      r.w = (int)MIN(dungeon->width * 2 - 1 - r.x, r.w);
      r.h = (int)MIN(dungeon->height * 2 - 1 - r.y, r.h);
      for (int x = (int)r.x; x < (int)(r.x + r.w); x++) {
        for (int y = (int)r.y; y < (int)(r.y + r.h); y++) {
          if (IN_RECTANGLE(x, y, dungeon->width * 2, dungeon->height * 2)) {
            dungeon->canopy->putPixel(x, y, TCODColor::black);
            SubCell* sub = dungeon->getSubCell(x, y);
            sub->shadow = sub->shadowBeforeTree;
            dungeon->setShadowHeight(x, y, dungeon->smapBeforeTree->getValue(x, y));
          }
        }
      }
      for (int x = (int)(it->x + treeRadius); x >= (int)(it->x - treeRadius); x--) {
        for (int y = (int)(it->y - treeRadius); y < (int)(it->y + treeRadius); y++) {
          if (IN_RECTANGLE(x, y, dungeon->width, dungeon->height)) {
            item::Item* tree = dungeon->getItem(x, y, "tree");
            if (tree) {
              setCanopy(x * 2, y * 2, tree->typeData, &r);
            }
          }
        }
      }
    } else {
      // reset the whole map
      dungeon->canopy->clear(TCODColor::black);
      dungeon->restoreShadowBeforeTree();
      for (int x = dungeon->width - 1; x >= 0; x--) {
        for (int y = 0; y < dungeon->height - 1; y++) {
          item::Item* tree = dungeon->getItem(x, y, "tree");
          if (tree) {
            setCanopy(x * 2, y * 2, tree->typeData);
          }
        }
      }
    }
  }
}

void GameEngine::setCanopy(int x, int y, const item::ItemType* treeType, const Rect* pr) {
  static TCODColor green1 = TCODColor::darkChartreuse;
  static TCODColor green2 = TCODColor::green * 0.7f;
  static const int treeRadiusW = config.getIntProperty("config.display.treeRadius");
  Rect r(0, 0, dungeon->width * 2, dungeon->height * 2);
  if (pr) r = *pr;
  for (int tx = -treeRadiusW; tx <= treeRadiusW; tx++) {
    if (x + tx >= r.x && x + tx < r.x + r.w) {
      // we want round trees even with non square fonts
      int dy = (int)(sqrtf(treeRadiusW * treeRadiusW - tx * tx) * aspectRatio);
      for (int ty = -dy; ty <= dy; ty++) {
        if (y + ty >= r.y && y + ty < r.y + r.h) {
          if (dungeon->getShadowHeight(x + tx, y + ty) < 2.0f) {
            TCODColor treecol = TCODColor::lerp(green1, green2, TCODRandom::getInstance()->getFloat(0.0f, 1.0f));
            if (strcmp(treeType->name, "pine tree") == 0) treecol = treecol * 0.75f;
            treecol = treecol * (0.6f + 0.4f * (tx + treeRadiusW) / (2 * treeRadiusW));
            if (strcmp(treeType->name, "apple tree") == 0 && TCODRandom::getInstance()->getInt(0, 80) == 0)
              treecol = TCODColor::darkOrange;
            dungeon->canopy->putPixel(x + tx, y + ty, treecol);
            if (x + tx >= 2) {
              // cast shadow
              dungeon->setShadowHeight(x + tx, y + ty, 2.0f);
              float shadow = dungeon->getShadow(x + tx - 2, y + ty) * 0.95f;
              dungeon->setShadow(x + tx - 2, y + ty, shadow);
              if (dungeon->getShadowHeight(x + tx - 2, y + ty) < 2.0f) {
                TCODColor col = dungeon->canopy->getPixel(x + tx - 2, y + ty);
                if (col.r != 0) {
                  col = col * shadow;
                  dungeon->canopy->putPixel(x + tx - 2, y + ty, col);
                }
              }
            }
          }
        }
      }
    }
  }
}

void GameEngine::computeAspectRatio() {
  int charw, charh;
  TCODSystem::getCharSize(&charw, &charh);
  aspectRatio = (float)(charw) / charh;
}

void GameEngine::hitFlash() {
  static float hitFlashDelay = config.getFloatProperty("config.display.hitFlashDelay");
  hitFlashAmount = hitFlashDelay;
}

TCODColor GameEngine::setSepia(const TCODColor& col, float coef) {
  float h, s, v;
  col.getHSV(&h, &s, &v);
  TCODColor ret = col;
  ret.setHSV(28.0f, 0.25f, v * 0.7f);
  return TCODColor::lerp(col, ret, coef);
}

bool GameEngine::update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) {
  static float hitFlashDelay = config.getFloatProperty("config.display.hitFlashDelay");
  static TCODColor flashColor = config.getColorProperty("config.display.flashColor");
  packer.clear();
  if (fade == FADE_OFF) {
    if (hitFlashAmount > 0.0f) {
      hitFlashAmount -= elapsed;
      if (hitFlashAmount > 0.0f) {
        int flashLvl = (int)(255 - 128 * hitFlashAmount / hitFlashDelay);
        TCODConsole::setFade(flashLvl, flashColor);
      } else {
        TCODConsole::setFade(255, flashColor);
      }
    }
  }
  if (isGamePaused() && pauseCoef != 1.0f) {
    pauseCoef += elapsed;
    if (pauseCoef > 1.0f) pauseCoef = 1.0f;
  } else if (!isGamePaused() && pauseCoef > 0.0f) {
    pauseCoef -= elapsed;
    if (pauseCoef < 0.0f) pauseCoef = 0.0f;
  }
  return true;
}

void GameEngine::init() {
  gameEngine = this;
  engine.setKeyboardMode(UMBRA_KEYBOARD_PRESSED_RELEASED);
  player.init();
}

void GameEngine::pauseGame() { nbPause++; }

void GameEngine::resumeGame() { nbPause--; }

void GameEngine::startRipple(int dungeonx, int dungeony, float height) {
  if (rippleManager) rippleManager->startRipple(dungeonx, dungeony, height);
}

void GameEngine::startFireZone(int x, int y, int w, int h) {
  if (fireManager) fireManager->addZone(x * 2, y * 2, w * 2, h * 2);
}

void GameEngine::removeFireZone(int x, int y, int w, int h) {
  if (fireManager) fireManager->removeZone(x * 2, y * 2, w * 2, h * 2);
}

void GameEngine::onInitialise() { gui.initialize(); }

void GameEngine::openCloseInventory() {
  if (gui.mode == GUI_INVENTORY || gui.mode == GUI_INVLOOT)
    gui.setMode(GUI_NONE);
  else if (gui.mode == GUI_LOOT)
    gui.setMode(GUI_INVLOOT);
  else
    gui.setMode(GUI_INVENTORY);
}

void GameEngine::openCloseObjectives() {
  if (gui.mode == GUI_OBJECTIVES)
    gui.setMode(GUI_NONE);
  else
    gui.setMode(GUI_OBJECTIVES);
}

void GameEngine::openCloseCraft() {
  if (gui.mode == GUI_CRAFT)
    gui.setMode(GUI_NONE);
  else
    gui.setMode(GUI_CRAFT);
}

void GameEngine::openCloseLoot(item::Item* toLoot) {
  if (gui.mode == GUI_LOOT)
    gui.setMode(GUI_NONE);
  else {
    gui.loot.initialize(toLoot);
    gui.setMode(GUI_INVLOOT);
  }
}

void GameEngine::addFireball(FireBall* fb) {
  if (isUpdatingFireballs)
    fireballsToAdd.push(fb);
  else
    fireballs.push(fb);
}

void GameEngine::updateFireballs(float elapsed) {
  TCODList<FireBall*> fireballsToRemove;
  if (!fireballsToAdd.isEmpty()) {
    fireballs.addAll(fireballsToAdd);
    fireballsToAdd.clear();
  }
  isUpdatingFireballs = true;
  for (FireBall** it = fireballs.begin(); it != fireballs.end(); it++) {
    if (!(*it)->update(elapsed)) {
      fireballsToRemove.push(*it);
      it = fireballs.removeFast(it);
    }
  }
  isUpdatingFireballs = false;
  fireballsToRemove.clearAndDelete();
}

void GameEngine::displayProgress(float prog) {
  // printf ("==> %g \n",prog);
  int l = (int)(CON_W / 2 * prog);
  if (l > 0) {
    TCODConsole::root->setDefaultBackground(TCODColor::red);
    TCODConsole::root->rect(CON_W / 4, CON_H / 2 - 1, l, 2, false, TCOD_BKGND_SET);
  }
  if (l < CON_W / 2) {
    TCODConsole::root->setDefaultBackground(TCODColor::darkestRed);
    TCODConsole::root->rect(CON_W / 4 + l, CON_H / 2 - 1, CON_W / 2 - l, 2, false, TCOD_BKGND_SET);
  }
  TCODConsole::root->flush();
  TCODConsole::checkForKeypress();
}
