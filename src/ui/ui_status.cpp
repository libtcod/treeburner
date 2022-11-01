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
#include "ui_status.hpp"

#include "constants.hpp"
#include "main.hpp"
#include "ui_inventory.hpp"
#include "util/subcell.hpp"

#define PANEL_HEIGHT 11

StatusPanel::StatusPanel() {
  con = new TCODConsole(12, PANEL_HEIGHT);
  con->setDefaultBackground(guiBackground);
  con->setDefaultForeground(guiText);
  con->clear();
  flags = DIALOG_MULTIPOS | DIALOG_DRAGGABLE;
  possiblePos.push(new UmbraRect(CON_W - 12, 0, 12, PANEL_HEIGHT));
  possiblePos.push(new UmbraRect(CON_W - 12, 0, 12, PANEL_HEIGHT));
  possiblePos.push(new UmbraRect(CON_W - 12, 0, 12, PANEL_HEIGHT));

  possiblePos.push(new UmbraRect(CON_W - 12, CON_H - PANEL_HEIGHT, 12, PANEL_HEIGHT));
  possiblePos.push(new UmbraRect(CON_W - 12, CON_H - PANEL_HEIGHT, 12, PANEL_HEIGHT));
  possiblePos.push(new UmbraRect(CON_W - 12, CON_H - PANEL_HEIGHT, 12, PANEL_HEIGHT));

  possiblePos.push(new UmbraRect(0, 0, 12, PANEL_HEIGHT));
  possiblePos.push(new UmbraRect(0, 0, 12, PANEL_HEIGHT));
  possiblePos.push(new UmbraRect(0, 0, 12, PANEL_HEIGHT));

  possiblePos.push(new UmbraRect(0, CON_H - PANEL_HEIGHT, 12, PANEL_HEIGHT));
  possiblePos.push(new UmbraRect(0, CON_H - PANEL_HEIGHT, 12, PANEL_HEIGHT));
  possiblePos.push(new UmbraRect(0, CON_H - PANEL_HEIGHT, 12, PANEL_HEIGHT));
  minimizedRect = *possiblePos.get(0);
  minimizedRect.x = userPref.statusx;
  minimizedRect.y = userPref.statusy;
  setMinimized();
  titleBarAlpha = 0.0f;
}

void StatusPanel::render() {
  static TCODImage img(20, 4);
  mob::Player* player = &gameEngine->player;
  int y, dy;
  if (rect.y == 0) {
    y = 1;
    dy = 1;
  } else {
    y = rect.h - 2;
    dy = -1;
  }

  if (titleBarAlpha > 0.0f) {
    con->clear();
    con->printEx(rect.w / 2, y, TCOD_BKGND_NONE, TCOD_CENTER, "HP %d/%d", (int)player->life, (int)player->maxLife);
    if (!player->conditions.isEmpty()) {
      con->printEx(rect.w / 2, y + 2 * dy, TCOD_BKGND_NONE, TCOD_CENTER, "Conditions");
    }
    blitSemiTransparent(con, 0, 0, rect.w, rect.h, TCODConsole::root, rect.x, rect.y, titleBarAlpha, titleBarAlpha);
  }
  if (titleBarAlpha > 0.0f) {
    renderFrame(titleBarAlpha, "Status");
  }

  int xhp = (int)(20 * player->getHealth());
  int xheal = (int)(20 * player->getHealing());
  for (int x = 0; x < xhp; x++) {
    img.putPixel(x, 0, TCODColor::red);
    img.putPixel(x, 1, TCODColor::red);
  }
  for (int x = xhp; x < xheal; x++) {
    img.putPixel(x, 0, TCODColor::darkRed);
    img.putPixel(x, 1, TCODColor::darkRed);
  }
  for (int x = xheal; x < 20; x++) {
    img.putPixel(x, 0, TCODColor::black);
    img.putPixel(x, 1, TCODColor::black);
  }
  img.blit2x(TCODConsole::root, rect.x + 1, rect.y + y + dy, 0, 0, 20, 2);

  y += 3 * dy;
  TCODList<mob::Condition*> conds;
  // extract conditions from the player.
  // every condition is displayed only once (the longer)
  for (mob::Condition** it = player->conditions.begin(); it != player->conditions.end(); it++) {
    mob::Condition** it2 = NULL;
    for (it2 = conds.begin(); it2 != conds.end(); it2++) {
      if ((*it)->equals((*it2)->type->type, (*it2)->alias)) {
        break;
      }
    }
    if (it2 == conds.end()) {
      // new condition
      conds.push(*it);
    } else {
      // replace only if longer
      if ((*it)->duration > (*it2)->duration) *it2 = *it;
    }
  }
  for (mob::Condition** it = conds.begin(); it != conds.end(); it++) {
    float coef = (*it)->duration / (*it)->initialDuration;
    for (int x = 0; x < 20; x++) {
      img.putPixel(x, 0, x < coef * 20 ? TCODColor::azure : TCODColor::black);
      img.putPixel(x, 1, x < coef * 20 ? TCODColor::azure : TCODColor::black);
    }
    img.blit2x(TCODConsole::root, rect.x + 1, rect.y + y, 0, 0, 20, 2);
    TCODConsole::root->setDefaultForeground(guiText);
    TCODConsole::root->printEx(
        rect.x + rect.w / 2, rect.y + y, TCOD_BKGND_NONE, TCOD_CENTER, (*it)->alias ? (*it)->alias : (*it)->type->name);
    y += dy;
  }
}

bool StatusPanel::update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse) {
  static bool lookOn = false;
  if (k.vk == TCODK_ALT || k.lalt) lookOn = k.pressed;
  if (!gameEngine->isGamePaused() && rect.mouseHover && !lookOn) {
    titleBarAlpha += elapsed;
    titleBarAlpha = MIN(1.0f, titleBarAlpha);
  } else if (!isDragging) {
    titleBarAlpha -= elapsed;
    titleBarAlpha = MAX(0.0f, titleBarAlpha);
  }
  return true;
}

void StatusPanel::setPos(int x, int y) {
  MultiPosDialog::setPos(x, y);
  userPref.statusx = x;
  userPref.statusy = y;
}
