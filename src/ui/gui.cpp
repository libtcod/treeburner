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
#include "ui/gui.hpp"

#include "main.hpp"

namespace ui {
void Gui::initialize() {
  engine.registerModule(&log);
  engine.registerModule(&statusPanel);
  engine.registerModule(&inventory);
  engine.registerModule(&loot);
  engine.registerModule(&tutorial);
  engine.registerModule(&objectives);
  engine.registerModule(&craft);

  // log and status panel are linked (moving one impacts the other)
  log.set.push(&statusPanel);
  statusPanel.set.push(&log);
  mode = GUI_NONE;
}

void Gui::activate() {
  engine.activateModule(&log);
  engine.activateModule(&statusPanel);
  tutorial.init();
  engine.activateModule(&tutorial);
  engine.activateModule(&objectives);
}

void Gui::deactivate() {
  setMode(GUI_NONE);
  engine.deactivateModule(&log);
  engine.deactivateModule(&statusPanel);
  engine.deactivateModule(&tutorial);
  engine.deactivateModule(&objectives);
}

void Gui::setMode(EGuiMode newMode) {
  if (newMode != GUI_INVLOOT || (mode != GUI_INVENTORY && mode != GUI_LOOT)) closeDialogs();
  mode = newMode;
  switch (mode) {
    case GUI_INVENTORY:
      inventory.initialize(&gameEngine->player);
      engine.activateModule(&inventory);
      break;
    case GUI_LOOT:
      engine.activateModule(&loot);
      break;
    case GUI_INVLOOT:
      inventory.initialize(&gameEngine->player);
      engine.activateModule(&inventory);
      engine.activateModule(&loot);
      break;
    case GUI_OBJECTIVES:
      objectives.setOnOff(true);
      break;
    case GUI_MAXIMIZED:
      break;
    case GUI_CRAFT:
      craft.initialize(&gameEngine->player);
      engine.activateModule(&craft);
      break;
    case GUI_TUTORIAL:
      tutorial.openMenu();
      break;
    case GUI_NONE:
      break;
  }
}

void Gui::closeDialogs() {
  switch (mode) {
    case GUI_INVENTORY:
      engine.deactivateModule(&inventory);
      break;
    case GUI_LOOT:
      engine.deactivateModule(&loot);
      break;
    case GUI_INVLOOT:
      engine.deactivateModule(&inventory);
      engine.deactivateModule(&loot);
      break;
    case GUI_OBJECTIVES:
      objectives.setOnOff(false);
      break;
    case GUI_MAXIMIZED:
      log.setMinimized();
      break;
    case GUI_CRAFT:
      engine.deactivateModule(&craft);
      break;
    case GUI_TUTORIAL:
      tutorial.closeLiveTuto();
      break;
    case GUI_NONE:
      break;
  }
}
}  // namespace ui
