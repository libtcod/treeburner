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
#include "ui/objectives.hpp"

#include "constants.hpp"
#include "main.hpp"
#include "util/subcell.hpp"

namespace ui {
#define OBJ_WIDTH (CON_W - 4)
#define OBJ_HEIGHT 40

Objective::Objective(const char* title, const char* description, const char* enableScript, const char* successScript)
    : title(title),
      description(description),
      enableScript(enableScript),
      successScript(successScript),
      onEnable(NULL),
      onSuccess(NULL) {
  if (enableScript) {
    onEnable = new util::Script();
    onEnable->parse(enableScript);
  }
  if (successScript) {
    onSuccess = new util::Script();
    onSuccess->parse(successScript);
  }
}

#define UPDATE_DELAY 3.0f

Objectives::Objectives() : timer(0.0f), showWindow(false), firstObj(true) {
  saveGame.registerListener(OBJE_CHUNK_ID, base::PHASE_START, this);
  rect = UmbraRect(2, 5, OBJ_WIDTH, OBJ_HEIGHT);
  con = new TCODConsole(OBJ_WIDTH, OBJ_HEIGHT);
  con->setDefaultBackground(ui::guiBackground);
  guiTabs.addTab("Active");
  guiTabs.addTab("Success");
  guiTabs.addTab("Failure");
  flags = ui::DIALOG_CLOSABLE_NODISABLE;
  currentList = &active;
  scroller = new ui::Scroller(this, OBJ_WIDTH / 2 - 1, OBJ_HEIGHT - 2);
  selected = 0;
}

bool Objectives::executeObjScript(Objective* obj, util::Script* script) {
  currentObjective = obj;
  bool ret = script->execute();
  currentObjective = NULL;
  return ret;
}

void Objectives::activateCurrent() {
  gameEngine->gui.log.warn("New objective : %s", currentObjective->title);
  if (firstObj) {
    gameEngine->gui.log.info("Press 'o' to open the objectives screen");
    firstObj = false;
  }
  toActivate.push(currentObjective);
}

void Objectives::activateObjective(const char* title) {
  for (Objective** it = sleeping.begin(); it != sleeping.end(); it++) {
    if (strcmp(title, (*it)->title) == 0) {
      gameEngine->gui.log.warn("New objective : %s", title);
      toActivate.push(*it);
      break;
    }
  }
}

void Objectives::closeCurrent(bool success) {
  gameEngine->gui.log.warn("Objective completed : %s (%s)", currentObjective->title, success ? "success" : "failure");
  if (success)
    toSuccess.push(currentObjective);
  else
    toFailure.push(currentObjective);
}

void Objectives::addStep(const char* msg, Objective* obj) {
  if (!obj) obj = currentObjective;
  if (!toSuccess.contains(obj)) gameEngine->gui.log.warn("Objective updated : %s", obj->title);
  obj->steps.push(strdup(msg));
}

void Objectives::render() {
  if (!showWindow) return;
  con->clear();
  con->setDefaultForeground(ui::guiText);
  con->vline(OBJ_WIDTH / 2, 2, OBJ_HEIGHT - 3);
  guiTabs.render(con, 0, 0);
  scroller->render(con, 0, 2);
  scroller->renderScrollbar(con, 0, 2);
  if (currentList && selected < currentList->size()) {
    con->setDefaultForeground(ui::guiText);
    int y = 2;
    Objective* objective = currentList->get(selected);
    y += con->printRect(OBJ_WIDTH / 2 + 2, y, OBJ_WIDTH / 2 - 3, 0, objective->description);
    for (const char** step = objective->steps.begin(); step != objective->steps.end(); step++) {
      y++;
      y += con->printRect(OBJ_WIDTH / 2 + 2, y, OBJ_WIDTH / 2 - 3, 0, *step);
    }
  }
  util::blitSemiTransparent(con, 0, 0, OBJ_WIDTH, OBJ_HEIGHT, TCODConsole::root, rect.x, rect.y, 0.8f, 1.0f);
  renderFrame(1.0f, "Objectives");
}

int Objectives::getScrollTotalSize() { return currentList->size(); }

const std::string& Objectives::getScrollText(int idx) { return currentList->get(idx)->title; }

void Objectives::getScrollColor(int idx, TCODColor* fore, TCODColor* back) {
  *fore = idx == selected ? ui::guiHighlightedText : ui::guiText;
  *back = ui::guiBackground;
}

bool Objectives::update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse) {
  if (showWindow) {
    flags |= ui::DIALOG_MODAL;
    guiTabs.update(elapsed, k, mouse, rect.x, rect.y);
    scroller->update(elapsed, k, mouse, rect.x, rect.y + 2);
    if (mouse.cx >= rect.x && mouse.cx < rect.x + rect.w / 2 && mouse.cy >= rect.y + 2 && mouse.cy < rect.y + rect.h) {
      int newSelected = mouse.cy - rect.y - 2;
      if (currentList && newSelected < currentList->size()) selected = newSelected;
    }
    switch (guiTabs.curTab) {
      case 0:
        currentList = &active;
        break;
      case 1:
        currentList = &success;
        break;
      case 2:
        currentList = &failed;
        break;
    }
    if (closeButton.mouseHover && mouse.lbutton_pressed) {
      gameEngine->gui.setMode(ui::GUI_NONE);
    }
    if ((k.vk == TCODK_ESCAPE && !k.pressed)) {
      gameEngine->gui.setMode(ui::GUI_NONE);
    }
  } else if (wasShowingWindow) {
    flags &= ~ui::DIALOG_MODAL;
    if (gameEngine->gui.mode == ui::GUI_NONE && gameEngine->isGamePaused()) {
      gameEngine->resumeGame();
    }
  }
  timer += elapsed;
  if (timer < UPDATE_DELAY) return true;
  timer = 0.0f;
  // check end conditions for active objectives
  for (Objective** it = active.begin(); it != active.end(); it++) {
    if (!(*it)->onSuccess)
      toSuccess.push(*it);
    else if (!executeObjScript(*it, (*it)->onSuccess)) {
      // script execution failed. junk the objective
      failed.push(*it);
      it = active.removeFast(it);
    }
  }
  // check if new objectives are enabled
  for (Objective** it = sleeping.begin(); it != sleeping.end(); it++) {
    if (!(*it)->onEnable) {
      toActivate.push(*it);
      gameEngine->gui.log.warn("New objective : %s", (*it)->title);
      if (firstObj) {
        gameEngine->gui.log.info("Press 'o' to open the objectives screen");
        firstObj = false;
      }
    } else if (!executeObjScript(*it, (*it)->onEnable)) {
      // script execution failed. junk the objective
      failed.push(*it);
      it = sleeping.removeFast(it);
    }
  }
  for (Objective** it = toActivate.begin(); it != toActivate.end(); it++) {
    sleeping.removeFast(*it);
    active.push(*it);
  }
  toActivate.clear();
  for (Objective** it = toSuccess.begin(); it != toSuccess.end(); it++) {
    active.removeFast(*it);
    success.push(*it);
  }
  toSuccess.clear();
  for (Objective** it = toFailure.begin(); it != toFailure.end(); it++) {
    active.removeFast(*it);
    failed.push(*it);
  }
  toFailure.clear();
  wasShowingWindow = showWindow;
  return true;
}

void Objectives::addObjective(Objective* obj) { sleeping.push(obj); }

#define OBJE_CHUNK_VERSION 1

bool Objectives::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) {
  if (chunkVersion != OBJE_CHUNK_VERSION) return false;
  showWindow = (zip->getChar() == 1);
  firstObj = (zip->getChar() == 1);
  int nbSleeping = zip->getInt();
  while (nbSleeping > 0) {
    const char* title = strdup(zip->getString());
    const char* description = strdup(zip->getString());
    const char* enableScript = zip->getString();
    if (enableScript) enableScript = strdup(enableScript);
    const char* successScript = zip->getString();
    if (successScript) successScript = strdup(successScript);
    Objective* obj = new Objective(title, description, enableScript, successScript);
    sleeping.push(obj);
    nbSleeping--;
  }
  int nbActive = zip->getInt();
  while (nbActive > 0) {
    const char* title = strdup(zip->getString());
    const char* description = strdup(zip->getString());
    const char* enableScript = zip->getString();
    if (enableScript) enableScript = strdup(enableScript);
    const char* successScript = zip->getString();
    if (successScript) successScript = strdup(successScript);
    Objective* obj = new Objective(title, description, enableScript, successScript);
    active.push(obj);
    nbActive--;
  }
  int nbSuccess = zip->getInt();
  while (nbSuccess > 0) {
    const char* title = strdup(zip->getString());
    const char* description = strdup(zip->getString());
    Objective* obj = new Objective(title, description);
    success.push(obj);
    nbSuccess--;
  }
  int nbFailed = zip->getInt();
  while (nbFailed > 0) {
    const char* title = strdup(zip->getString());
    const char* description = strdup(zip->getString());
    Objective* obj = new Objective(title, description);
    failed.push(obj);
    nbFailed--;
  }
  return true;
}

void Objectives::saveData(uint32_t chunkId, TCODZip* zip) {
  saveGame.saveChunk(OBJE_CHUNK_ID, OBJE_CHUNK_VERSION);
  zip->putChar(showWindow ? 1 : 0);
  zip->putChar(firstObj ? 1 : 0);
  zip->putInt(sleeping.size());
  for (Objective** it = sleeping.begin(); it != sleeping.end(); it++) {
    zip->putString((*it)->title);
    zip->putString((*it)->description);
    zip->putString((*it)->enableScript);
    zip->putString((*it)->successScript);
  }
  zip->putInt(active.size());
  for (Objective** it = active.begin(); it != active.end(); it++) {
    zip->putString((*it)->title);
    zip->putString((*it)->description);
    zip->putString((*it)->enableScript);
    zip->putString((*it)->successScript);
  }
  zip->putInt(success.size());
  for (Objective** it = success.begin(); it != success.end(); it++) {
    zip->putString((*it)->title);
    zip->putString((*it)->description);
  }
  zip->putInt(failed.size());
  for (Objective** it = failed.begin(); it != failed.end(); it++) {
    zip->putString((*it)->title);
    zip->putString((*it)->description);
  }
}
}  // namespace ui
