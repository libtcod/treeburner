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
#include "ui/descriptor.hpp"

#include <stdio.h>

#include "main.hpp"

namespace ui {
#define POSX CON_W - 1
#define POSY CON_H - 10

Descriptor::Descriptor() : item(NULL), creature(NULL) { tooltip[0] = 0; }

void Descriptor::setFocus(int mousex, int mousey, int x, int y, bool lookOn) {
  this->x = x;
  this->y = y;
  this->mousex = mousex;
  this->mousey = mousey;
  map::Dungeon* dungeon = gameEngine->dungeon;
  mob::Player* player = &gameEngine->player;
  tooltip[0] = 0;
  item = NULL;
  creature = NULL;
  this->lookOn = lookOn;
  if (IN_RECTANGLE(x, y, dungeon->width, dungeon->height) && player->isInRange(x, y) && dungeon->map->isInFov(x, y)) {
    if (x == gameEngine->player.x_ && y == gameEngine->player.y_)
      creature = &gameEngine->player;
    else
      creature = dungeon->getCreature(x, y);
    if (!creature) item = dungeon->getFirstItem(x, y);
    if (lookOn && !creature && !item) {
      map::TerrainId id = dungeon->getTerrainType(x, y);
      strcpy(tooltip, map::terrainTypes[id].name);
    }
  } else if (lookOn) {
    map::TerrainId id = dungeon->getTerrainType(x, y);
    strcpy(tooltip, map::terrainTypes[id].name);
  }
}

void Descriptor::render() {
  TCODConsole::root->setDefaultForeground(ui::guiText);
  // descriptor
  if (creature) {
    TCODConsole::root->printEx(POSX, POSY, TCOD_BKGND_NONE, TCOD_RIGHT, creature->name_.c_str());
  } else if (item) {
    TCODConsole::root->printEx(POSX, POSY, TCOD_BKGND_NONE, TCOD_RIGHT, item->aName().c_str());
    if (lookOn) item->renderDescription(mousex, mousey - 1, false);
  }
  // tooltip
  if (tooltip[0] != 0) {
    int my = mousey - 1;
    if (my < 0) my = 2;
    if (IN_RECTANGLE(mousex, my, CON_W, CON_H)) {
      TCODConsole::root->printEx(mousex, my, TCOD_BKGND_NONE, TCOD_CENTER, tooltip);
    }
  }
}
}  // namespace ui
